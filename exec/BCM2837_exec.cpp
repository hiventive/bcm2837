
//#include <gtest/gtest.h>
#include <BCM2837.h>
#include <button.h>
#include <led.h>
#include <uart-backend.h>
#include <button-backend.h>
#include <led-backend.h>

#define HV_SET_LOCATION(a, s)                                                                      \
    ::std::string("{\"address\": ") + ::std::to_string(a) + ", \"size\": " + ::std::to_string(s) + \
        "}"

#define HV_BCM2837_EXEC_MODULE_NAME "BCM2837Exec"

#define RASPI3_RESET_CBAR 0x3f000000
#define RASPI3_BOARD_ID 0xc44
#define RASPI3_SMPBOOT_ADDR 0x300
#define RASPI3_MVBAR_ADDR 0x400
#define RASPI3_SPINTABLE_ADDR 0xd8
#define IF_SD 6

#define WEBSOCKET_SERVER_PORT 3030

#define RASPI3_RAM_SIZE 0x40000000
#define RASPI3_VCRAM_SIZE 0x01000000

class BCM2837ExecModule : public ::hv::module::Module {
  public:
    BCM2837ExecModule(::hv::module::ModuleName name_)
        : ::hv::module::Module(name_), mBCM2837("BCM2837"), mWSServer(WEBSOCKET_SERVER_PORT, true),
          mButton("Button0"), mLED("LED0"), mUARTBackend("UARTBackend"),
          mButtonBackend("ButtonBackend", mButton), mLEDBackend("LEDBackend", mLED) {
        SC_HAS_PROCESS(BCM2837ExecModule);

        // Registering UART backend to WebSocket server
        ::std::cout << "Registering UART backend..." << ::std::endl;
        mWSServer.registerBackend(&mUARTBackend);
        // Registering button backend to WebSocket server
        ::std::cout << "Registering button backend..." << ::std::endl;
        mWSServer.registerBackend(&mButtonBackend);
        // Registering LED backend to WebSocket server
        ::std::cout << "Registering LED backend..." << ::std::endl;
        mWSServer.registerBackend(&mLEDBackend);

        // Connecting UART backend socket to GPIO
        ::std::pair<::hv::gpio_socket_id_t, ::hv::gpio_socket_id_t> gpioUARTSocketId =
            mBCM2837.mGPIO.connectSocket(mUARTBackend.socket);

        // Connecting button socket to GPIO
        ::hv::gpio_socket_id_t gpioButtonSocketId =
            mBCM2837.mGPIO.connectSocket(mButton.outputSocket);

        // Connecting LED socket to GPIO
        ::hv::gpio_socket_id_t gpioLEDSocketId = mBCM2837.mGPIO.connectSocket(mLED.socketPin0);

        // Setting UART RX/TX to pins 32/33
        ::std::cout << "Connecting UART backend to GPIO pins 32 and 33..." << ::std::endl;
        mBCM2837.mGPIO.connectIO(gpioUARTSocketId.second, "RX", 32);
        mBCM2837.mGPIO.connectIO(gpioUARTSocketId.first, "TX", 33);

        // Setting button to pin 9
        mBCM2837.mGPIO.connectIO(gpioButtonSocketId, "IO_OUT", 9);
        mBCM2837.mGPIO.connectIO(gpioLEDSocketId, "IO_IN", 10);

        ::std::cout << "Starting WebSocket server..." << ::std::endl << ::std::flush;
        mWSServer.startServer();

        SC_THREAD(keepAlive);
    }

  protected:
    ::hv::BCM2837<64> mBCM2837;

    // WebSocket server
    ::hv::backend::HVWSS mWSServer;

    // Button
    ::hv::dev::Button mButton;

    // LED
    ::hv::dev::LED mLED;

    // Backends
    ::hv::backend::UARTBackend mUARTBackend;
    ::hv::backend::ButtonBackend mButtonBackend;
    ::hv::backend::LEDBackend mLEDBackend;

    void keepAlive() {
        while (1) {
            wait(::sc_core::SC_ZERO_TIME);
        }
    }
};

int sc_main(int argc, char *argv[]) {
    ::hv::cfg::Broker mBroker("MyGlobalBroker");

    srand(time(NULL));

    ::std::string kernel("");
    ::std::string kernelCmd("");
    ::std::string initrd("");
    ::std::string dtb("");
    ::std::string sd("");
    bool gdbActivated = false;
    ::hv::common::hvuint16_t gdbPort = 1234;

    int argTmp = 1;
    while (argTmp < argc) {
        if (::std::string("-kernel") == argv[argTmp]) {
            argTmp++;
            HV_ASSERT(argTmp < argc, "-kernel: missing an argument")
            HV_ASSERT(argv[argTmp][0] != '-', "-kernel: missing an argument")
            kernel = argv[argTmp];
        } else if (::std::string("-kernel_cmd") == argv[argTmp]) {
            argTmp++;
            HV_ASSERT(argTmp < argc, "-kernel_cmd: missing an argument")
            HV_ASSERT(argv[argTmp][0] != '-', "-kernel_cmd: missing an argument")
            kernelCmd = argv[argTmp];
        } else if (::std::string("-initrd") == argv[argTmp]) {
            argTmp++;
            HV_ASSERT(argTmp < argc, "-initrd: missing an argument")
            HV_ASSERT(argv[argTmp][0] != '-', "-initrd: missing an argument")
            initrd = argv[argTmp];
        } else if (::std::string("-dtb") == argv[argTmp]) {
            argTmp++;
            HV_ASSERT(argTmp < argc, "-dtb: missing an argument")
            HV_ASSERT(argv[argTmp][0] != '-', "-dtb: missing an argument")
            dtb = argv[argTmp];
        } else if (::std::string("-gdb") == argv[argTmp]) {
            argTmp++;
            gdbActivated = true;
            if (argTmp < argc) {
                if (argv[argTmp][0] != '-') {
                    gdbPort = static_cast<::hv::common::hvuint16_t>(atoi(argv[argTmp]));
                }
            } else {
                argTmp--;
            }
        } else if (::std::string("-sd") == argv[argTmp]) {
            argTmp++;
            HV_ASSERT(argTmp < argc, "-sd: missing an argument")
            HV_ASSERT(argv[argTmp][0] != '-', "-sd: missing an argument")
            sd = argv[argTmp];
        } else {
            HV_ERR("Unknown argument " << argv[argTmp])
        }
        argTmp++;
    }

    if (gdbActivated) {
        mBroker.getCCIBroker().set_preset_cci_value(
            HV_BCM2837_EXEC_MODULE_NAME ".BCM2837.activateGDBServer", ::cci::cci_value(true),
            cci::cci_originator("sc_main"));
        mBroker.getCCIBroker().set_preset_cci_value(HV_BCM2837_EXEC_MODULE_NAME ".BCM2837.gdbPort",
                                                    ::cci::cci_value(gdbPort),
                                                    cci::cci_originator("sc_main"));
    }

    // CPU
    mBroker.getCCIBroker().set_preset_cci_value(HV_BCM2837_EXEC_MODULE_NAME ".BCM2837.resetCBAR",
                                                ::cci::cci_value(RASPI3_RESET_CBAR),
                                                cci::cci_originator("sc_main"));
    mBroker.getCCIBroker().set_preset_cci_value(HV_BCM2837_EXEC_MODULE_NAME ".BCM2837.RAMSize",
                                                ::cci::cci_value(RASPI3_RAM_SIZE),
                                                cci::cci_originator("sc_main"));
    mBroker.getCCIBroker().set_preset_cci_value(HV_BCM2837_EXEC_MODULE_NAME ".BCM2837.VCRAMSize",
                                                ::cci::cci_value(RASPI3_VCRAM_SIZE),
                                                cci::cci_originator("sc_main"));
    mBroker.getCCIBroker().set_preset_cci_value(HV_BCM2837_EXEC_MODULE_NAME ".BCM2837.boardId",
                                                ::cci::cci_value(RASPI3_BOARD_ID),
                                                cci::cci_originator("sc_main"));
    mBroker.getCCIBroker().set_preset_cci_value(HV_BCM2837_EXEC_MODULE_NAME ".BCM2837.SMPBootAddr",
                                                ::cci::cci_value(RASPI3_SMPBOOT_ADDR),
                                                cci::cci_originator("sc_main"));
    mBroker.getCCIBroker().set_preset_cci_value(HV_BCM2837_EXEC_MODULE_NAME ".BCM2837.kernel",
                                                ::cci::cci_value(kernel),
                                                cci::cci_originator("sc_main"));
    mBroker.getCCIBroker().set_preset_cci_value(HV_BCM2837_EXEC_MODULE_NAME ".BCM2837.initrd",
                                                ::cci::cci_value(initrd),
                                                cci::cci_originator("sc_main"));
    mBroker.getCCIBroker().set_preset_cci_value(HV_BCM2837_EXEC_MODULE_NAME ".BCM2837.kernelCmd",
                                                ::cci::cci_value(kernelCmd),
                                                cci::cci_originator("sc_main"));
    mBroker.getCCIBroker().set_preset_cci_value(HV_BCM2837_EXEC_MODULE_NAME ".BCM2837.dtb",
                                                ::cci::cci_value(dtb),
                                                cci::cci_originator("sc_main"));
    mBroker.getCCIBroker().set_preset_cci_value(HV_BCM2837_EXEC_MODULE_NAME ".BCM2837.sd",
                                                ::cci::cci_value(sd),
                                                cci::cci_originator("sc_main"));

    // BCM2835-ARMControl
    ::cci::cci_value_map ARMControlLocation;
    ARMControlLocation["address"].set(0x3f00b000);
    ARMControlLocation["size"].set(0x228);
    mBroker.getCCIBroker().set_preset_cci_value(
        HV_BCM2837_EXEC_MODULE_NAME ".BCM2837.ARMControl.MemMapSocket.location",
        ::cci::cci_value(ARMControlLocation), cci::cci_originator("sc_main"));

    // BCM2836-Control
    ::cci::cci_value_map ControlLocation;
    ControlLocation["address"].set(0x40000000);
    ControlLocation["size"].set(0x100);
    mBroker.getCCIBroker().set_preset_cci_value(
        HV_BCM2837_EXEC_MODULE_NAME ".BCM2837.Control.MemMapSocket.location",
        ::cci::cci_value(ControlLocation), cci::cci_originator("sc_main"));

    // UART0
    ::cci::cci_value_map UART0Location;
    UART0Location["size"].set(0x90);
    UART0Location["address"].set(0x3f201000);
    mBroker.getCCIBroker().set_preset_cci_value(
        HV_BCM2837_EXEC_MODULE_NAME ".BCM2837.UART0.MemMapSocket.location",
        ::cci::cci_value(UART0Location), cci::cci_originator("sc_main"));
    mBroker.getCCIBroker().set_preset_cci_value(HV_BCM2837_EXEC_MODULE_NAME ".BCM2837.UART0.IRQId",
                                                ::cci::cci_value(57),
                                                cci::cci_originator("sc_main"));

    // BCM2835-GPIO
    ::cci::cci_value_map GPIOLocation;
    GPIOLocation["address"].set(0x3f200000);
    GPIOLocation["size"].set(0xb4);
    mBroker.getCCIBroker().set_preset_cci_value(
        HV_BCM2837_EXEC_MODULE_NAME ".BCM2837.GPIO.MemMapSocket.location",
        ::cci::cci_value(GPIOLocation), cci::cci_originator("sc_main"));

    mBroker.getCCIBroker().set_preset_cci_value(
        HV_BCM2837_EXEC_MODULE_NAME ".BCM2837.GPIO.IRQMainId", ::cci::cci_value(49),
        cci::cci_originator("sc_main"));
    mBroker.getCCIBroker().set_preset_cci_value(HV_BCM2837_EXEC_MODULE_NAME ".BCM2837.GPIO.IRQ0Id",
                                                ::cci::cci_value(50),
                                                cci::cci_originator("sc_main"));
    mBroker.getCCIBroker().set_preset_cci_value(HV_BCM2837_EXEC_MODULE_NAME ".BCM2837.GPIO.IRQ1Id",
                                                ::cci::cci_value(51),
                                                cci::cci_originator("sc_main"));

    // CPRMAN Tweak
    mBroker.getCCIBroker().set_preset_cci_value(
        HV_BCM2837_EXEC_MODULE_NAME ".BCM2837.CPRMANTweak.Size", ::cci::cci_value(0x2000),
        cci::cci_originator("sc_main"));
    ::cci::cci_value_map CPRMANTweakLocation;
    CPRMANTweakLocation["address"].set(0x3f101000);
    CPRMANTweakLocation["size"].set(0x2000);
    mBroker.getCCIBroker().set_preset_cci_value(
        HV_BCM2837_EXEC_MODULE_NAME ".BCM2837.CPRMANTweak.Socket.location",
        ::cci::cci_value(CPRMANTweakLocation), cci::cci_originator("sc_main"));

    // Setting up button
    mBroker.getCCIBroker().set_preset_cci_value(
        HV_BCM2837_EXEC_MODULE_NAME ".Button0.outputWhenPushed", ::cci::cci_value(1),
        cci::cci_originator("sc_main"));
    mBroker.getCCIBroker().set_preset_cci_value(
        HV_BCM2837_EXEC_MODULE_NAME ".Button0.pullUpResistorSetup", ::cci::cci_value(0),
        cci::cci_originator("sc_main"));

    // Setting up led
    mBroker.getCCIBroker().set_preset_cci_value(
        HV_BCM2837_EXEC_MODULE_NAME ".LED0.pin1DefaultValue", ::cci::cci_value(0),
        cci::cci_originator("sc_main"));
    std::cout << "Instantiating BCM2837 exec module..." << std::endl;
    BCM2837ExecModule mBCM2837ExecModule(HV_BCM2837_EXEC_MODULE_NAME);

    std::cout << "Starting simulation..." << std::endl;
    ::sc_core::sc_start();
    std::cout << "End of simulation." << std::endl;
    return 0;
}
