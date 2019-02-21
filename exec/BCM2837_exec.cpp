
//#include <gtest/gtest.h>
#include <iostream>
#include <BCM2837.h>
#include <HVConfiguration>
#include <uart-ws.h>

#define HV_SET_LOCATION(a, s)                                                                      \
    ::std::string("{\"address\": ") + ::std::to_string(a) + ", \"size\": " + ::std::to_string(s) + \
        "}"

#define HV_BCM2837_TEST_MODULE_NAME "BCM2837Test"

#define RASPI3_RESET_CBAR 0x3f000000
#define RASPI3_BOARD_ID 0xc44
#define RASPI3_SMPBOOT_ADDR 0x300
#define RASPI3_MVBAR_ADDR 0x400
#define RASPI3_SPINTABLE_ADDR 0xd8
#define IF_SD 6

#define UART_WS_PORT 3030

#define RASPI3_RAM_SIZE 0x40000000
#define RASPI3_VCRAM_SIZE 0x01000000

class BCM2837TestModule : public ::hv::module::Module {
  public:
    BCM2837TestModule(::hv::module::ModuleName name_)
        : ::hv::module::Module(name_), mBCM2837("BCM2837"), mUARTWS("UARTWS", UART_WS_PORT) {
        ::std::pair<::hv::gpio_socket_id_t, ::hv::gpio_socket_id_t> socketID =
            mBCM2837.mGPIO.connectSocket(mUARTWS.getSocket());

        mBCM2837.mGPIO.connectIO(socketID.second, "RX", 32);
        mBCM2837.mGPIO.connectIO(socketID.first, "TX", 33);
    }

  protected:
    ::hv::BCM2837<64> mBCM2837;
    ::hv::backend::UARTWS mUARTWS;
};

int sc_main(int argc, char *argv[]) {
    ::hv::cfg::Broker mBroker("MyGlobalBroker");

    srand(time(NULL));

    ::std::string kernel("");
    ::std::string kernelCmd("");
    ::std::string initrd("");
    ::std::string dtb("");
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
        }
        argTmp++;
    }

    if (gdbActivated) {
        mBroker.getCCIBroker().set_preset_cci_value(
            HV_BCM2837_TEST_MODULE_NAME ".BCM2837.activateGDBServer", ::cci::cci_value(true),
            cci::cci_originator("sc_main"));
        mBroker.getCCIBroker().set_preset_cci_value(HV_BCM2837_TEST_MODULE_NAME ".BCM2837.gdbPort",
                                                    ::cci::cci_value(gdbPort),
                                                    cci::cci_originator("sc_main"));
    }

    // CPU
    mBroker.getCCIBroker().set_preset_cci_value(HV_BCM2837_TEST_MODULE_NAME ".BCM2837.resetCBAR",
                                                ::cci::cci_value(RASPI3_RESET_CBAR),
                                                cci::cci_originator("sc_main"));
    mBroker.getCCIBroker().set_preset_cci_value(HV_BCM2837_TEST_MODULE_NAME ".BCM2837.RAMSize",
                                                ::cci::cci_value(RASPI3_RAM_SIZE),
                                                cci::cci_originator("sc_main"));
    mBroker.getCCIBroker().set_preset_cci_value(HV_BCM2837_TEST_MODULE_NAME ".BCM2837.VCRAMSize",
                                                ::cci::cci_value(RASPI3_VCRAM_SIZE),
                                                cci::cci_originator("sc_main"));
    mBroker.getCCIBroker().set_preset_cci_value(HV_BCM2837_TEST_MODULE_NAME ".BCM2837.boardId",
                                                ::cci::cci_value(RASPI3_BOARD_ID),
                                                cci::cci_originator("sc_main"));
    mBroker.getCCIBroker().set_preset_cci_value(HV_BCM2837_TEST_MODULE_NAME ".BCM2837.SMPBootAddr",
                                                ::cci::cci_value(RASPI3_SMPBOOT_ADDR),
                                                cci::cci_originator("sc_main"));
    mBroker.getCCIBroker().set_preset_cci_value(HV_BCM2837_TEST_MODULE_NAME ".BCM2837.kernel",
                                                ::cci::cci_value(kernel),
                                                cci::cci_originator("sc_main"));
    mBroker.getCCIBroker().set_preset_cci_value(HV_BCM2837_TEST_MODULE_NAME ".BCM2837.initrd",
                                                ::cci::cci_value(initrd),
                                                cci::cci_originator("sc_main"));
    mBroker.getCCIBroker().set_preset_cci_value(HV_BCM2837_TEST_MODULE_NAME ".BCM2837.kernelCmd",
                                                ::cci::cci_value(kernelCmd),
                                                cci::cci_originator("sc_main"));
    mBroker.getCCIBroker().set_preset_cci_value(HV_BCM2837_TEST_MODULE_NAME ".BCM2837.dtb",
                                                ::cci::cci_value(dtb),
                                                cci::cci_originator("sc_main"));

    // BCM2835-ARMControl
    ::cci::cci_value_map ARMControlLocation;
    ARMControlLocation["address"].set(0x3f00b000);
    ARMControlLocation["size"].set(0x228);
    mBroker.getCCIBroker().set_preset_cci_value(
        HV_BCM2837_TEST_MODULE_NAME ".BCM2837.ARMControl.MemMapSocket.location",
        ::cci::cci_value(ARMControlLocation), cci::cci_originator("sc_main"));

    // BCM2836-Control
    ::cci::cci_value_map ControlLocation;
    ControlLocation["address"].set(0x40000000);
    ControlLocation["size"].set(0x100);
    mBroker.getCCIBroker().set_preset_cci_value(
        HV_BCM2837_TEST_MODULE_NAME ".BCM2837.Control.MemMapSocket.location",
        ::cci::cci_value(ControlLocation), cci::cci_originator("sc_main"));

    // UART0
    ::cci::cci_value_map UART0Location;
    UART0Location["size"].set(0x90);
    UART0Location["address"].set(0x3f201000);
    mBroker.getCCIBroker().set_preset_cci_value(
        HV_BCM2837_TEST_MODULE_NAME ".BCM2837.UART0.MemMapSocket.location",
        ::cci::cci_value(UART0Location), cci::cci_originator("sc_main"));
    mBroker.getCCIBroker().set_preset_cci_value(HV_BCM2837_TEST_MODULE_NAME ".BCM2837.UART0.IRQId",
                                                ::cci::cci_value(57),
                                                cci::cci_originator("sc_main"));

    // BCM2835-GPIO
    ::cci::cci_value_map GPIOLocation;
    GPIOLocation["address"].set(0x3f200000);
    GPIOLocation["size"].set(0xb4);
    mBroker.getCCIBroker().set_preset_cci_value(
        HV_BCM2837_TEST_MODULE_NAME ".BCM2837.GPIO.MemMapSocket.location",
        ::cci::cci_value(GPIOLocation), cci::cci_originator("sc_main"));

    mBroker.getCCIBroker().set_preset_cci_value(
        HV_BCM2837_TEST_MODULE_NAME ".BCM2837.GPIO.IRQMainId", ::cci::cci_value(49),
        cci::cci_originator("sc_main"));
    mBroker.getCCIBroker().set_preset_cci_value(HV_BCM2837_TEST_MODULE_NAME ".BCM2837.GPIO.IRQ0Id",
                                                ::cci::cci_value(50),
                                                cci::cci_originator("sc_main"));
    mBroker.getCCIBroker().set_preset_cci_value(HV_BCM2837_TEST_MODULE_NAME ".BCM2837.GPIO.IRQ1Id",
                                                ::cci::cci_value(51),
                                                cci::cci_originator("sc_main"));

    // CPRMAN Tweak
    mBroker.getCCIBroker().set_preset_cci_value(
        HV_BCM2837_TEST_MODULE_NAME ".BCM2837.CPRMANTweak.Size", ::cci::cci_value(0x2000),
        cci::cci_originator("sc_main"));
    ::cci::cci_value_map CPRMANTweakLocation;
    CPRMANTweakLocation["address"].set(0x3f101000);
    CPRMANTweakLocation["size"].set(0x2000);
    mBroker.getCCIBroker().set_preset_cci_value(
        HV_BCM2837_TEST_MODULE_NAME ".BCM2837.CPRMANTweak.Socket.location",
        ::cci::cci_value(CPRMANTweakLocation), cci::cci_originator("sc_main"));

    std::cout << "Instantiating BCM2837 test module..." << std::endl;
    BCM2837TestModule mBCM2837TestModule(HV_BCM2837_TEST_MODULE_NAME);

    std::cout << "Starting simulation..." << std::endl;
    ::sc_core::sc_start();
    std::cout << "End of simulation." << std::endl;
    return 0;
}
