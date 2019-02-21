/**
 * @file BCM2837.hpp
 * @author Benjamin Barrois <benjamin.barrois@hiventive.com>
 * @date Sep, 2018
 * @copyright Copyright (C) 2018, Hiventive.
 *
 * @brief Broadcom BCM2837
 */

#define GTIMER_PHYS 0
#define GTIMER_VIRT 1
#define GTIMER_HYP 2
#define GTIMER_SEC 3

#define ARM_CPU_IRQ 0
#define ARM_CPU_FIQ 1
#define ARM_CPU_VIRQ 2
#define ARM_CPU_VFIQ 3

namespace hv {

static const ::std::string smpbootName = "smpboot";
static const ::hv::common::hvuint32_t smpboot[11] = {0xd2801b05, 0xd53800a6, 0x924004c6, 0xd503205f,
                                                     0xf86678a4, 0xb4ffffc4, 0xd2800000, 0xd2800001,
                                                     0xd2800002, 0xd2800003, 0xd61f0080};

static const ::std::string spintablesName = "spintables";
static const ::hv::common::hvuint64_t spintables[4] = {0, 0, 0, 0};

template <unsigned int TLM_BUSWIDTH>
BCM2837<TLM_BUSWIDTH>::BCM2837(::hv::module::ModuleName name_)
    : Module(name_), mGPIO("GPIO"), resetCBAR("resetCBAR", 0u), ramSize("RAMSize", 0),
      vcramSize("VCRAMSize", 0), boardId("boardId", 0), smpBootAddr("SMPBootAddr", 0ul),
      kernelPath("kernel", ""), kernelCmd("kernelCmd", ""), initrdPath("initrd", ""),
      dtbPath("dtb", ""), activateGDBServer("activateGDBServer", false), gdbPort("gdbPort", 0u),
      cpuName(BCM2837_CPU_NAME), mCPU("CPU"), mMemoryMappedRouter("MemoryMappedRouter"),
      mUART0("UART0"), mControl("Control"), mARMControl("ARMControl"), mCPRMANTweak("CPRMANTweak"),
      mARMCtrlClog("ARMCtrlClog") {
    SC_HAS_PROCESS(BCM2837<TLM_BUSWIDTH>);

    //** Setting up QMG **//
    if (activateGDBServer.getValue()) {
        QMGEnableGDBServer();
        QMGSetGDBPort(gdbPort.getValue());
        QMGEnableGDBWaitForConnection();
    }
    QMGCPUDevice *cpuDevs[BCM2837_N_CPUS];
    // QMGIRQ* timerOutputIRQs[4 * BCM2837_N_CPUS];
    // QMGIRQ* cpuInputIRQs[4 * BCM2837_N_CPUS];
    // for (int i = 0; i < BCM2837_N_CPUS; ++i) {
    //     cpuDevs[i] = QMGAddCPU(cpuName.c_str(), false, i, resetCBAR.getValue());
    //     timerOutputIRQs[4 * i] = QMGCaptureOutputIRQ(&cpuDevs[i]->dev.base, GTIMER_PHYS);
    //     timerOutputIRQs[4 * i + 1] = QMGCaptureOutputIRQ(&cpuDevs[i]->dev.base, GTIMER_VIRT);
    //     timerOutputIRQs[4 * i + 2] = QMGCaptureOutputIRQ(&cpuDevs[i]->dev.base, GTIMER_HYP);
    //     timerOutputIRQs[4 * i + 3] = QMGCaptureOutputIRQ(&cpuDevs[i]->dev.base, GTIMER_SEC);

    //     cpuInputIRQs[4 * i] = QMGReachInputIRQ(&cpuDevs[i]->dev.base, ARM_CPU_IRQ);
    //     cpuInputIRQs[4 * i + 1] = QMGReachInputIRQ(&cpuDevs[i]->dev.base, ARM_CPU_FIQ);
    //     cpuInputIRQs[4 * i + 2] = QMGReachInputIRQ(&cpuDevs[i]->dev.base, ARM_CPU_VIRQ);
    //     cpuInputIRQs[4 * i + 3] = QMGReachInputIRQ(&cpuDevs[i]->dev.base, ARM_CPU_VFIQ);
    // }
    for (int i = 0; i < BCM2837_N_CPUS; ++i) {
        cpuDevs[i] = QMGAddCPU(cpuName.c_str(), false, i, resetCBAR.getValue());
        QMGCaptureOutputIRQ(&cpuDevs[i]->dev.base, GTIMER_PHYS);
        QMGCaptureOutputIRQ(&cpuDevs[i]->dev.base, GTIMER_HYP);
        QMGCaptureOutputIRQ(&cpuDevs[i]->dev.base, GTIMER_VIRT);
        QMGCaptureOutputIRQ(&cpuDevs[i]->dev.base, GTIMER_SEC);

        QMGReachInputIRQ(&cpuDevs[i]->dev.base, ARM_CPU_IRQ);
        QMGReachInputIRQ(&cpuDevs[i]->dev.base, ARM_CPU_FIQ);
        QMGReachInputIRQ(&cpuDevs[i]->dev.base, ARM_CPU_VIRQ);
        QMGReachInputIRQ(&cpuDevs[i]->dev.base, ARM_CPU_VFIQ);
    }

    QMGSetRAMSize(ramSize.getValue());
    QMGSetVCRAMSize(vcramSize.getValue());
    QMGSetBoardId(boardId.getValue());
    QMGSetSMPBootAddr(smpBootAddr.getValue());
    QMGSetSecondaryResetSetPCToSMPBootAddr(true);

    QMGSetBlockInterfaceType(::hv::QMG2SCBlockInterfaceType::QMG2SC_IF_SD);
    QMGEnableNoParallel();
    QMGEnableNoFloppy();
    QMGEnableNoCDROM();
    QMGSetIgnoreMemoryTransactionFailures(false);

    QMGSetKernelFilePath(kernelPath.getValue().c_str());
    QMGSetKernelCommand(kernelCmd.getValue().c_str());
    QMGSetInitRDFilePath(initrdPath.getValue().c_str());
    QMGSetDTBFilePath(dtbPath.getValue().c_str());

    QMGAddBlob((::hv::common::hvuint8_t *)&smpboot[0], sizeof(smpboot), smpbootName.c_str(),
               smpBootAddr.getValue());

    QMGAddBlob((::hv::common::hvuint8_t *)&spintables[0], sizeof(spintables),
               spintablesName.c_str(), 0xd8);

    //** Memory-Mapping **//
    mMemoryMappedRouter.outputSocket.bind(mUART0.memMapSocket);
    mMemoryMappedRouter.outputSocket.bind(mControl.memMapSocket);
    mMemoryMappedRouter.outputSocket.bind(mARMControl.memMapSocket);
    mMemoryMappedRouter.outputSocket.bind(mGPIO.memMapSocket);
    mMemoryMappedRouter.outputSocket.bind(mCPRMANTweak.socket);

    //** IRQ Mapping **//
    mUART0.IRQSocket.bind(mARMControl.GPUIRQIn);
    mGPIO.IRQSocket.bind(mARMControl.GPUIRQIn);

    //** Mapping ARMControl IRQ and FIQ to BCM Control
    mARMControl.IRQOut.bind(mControl.IRQInSocket);
    mARMControl.FIQOut.bind(mControl.FIQInSocket);

    //** Aggregating IRQ Outputs **//
    mControl.IRQOutSocket[0].bind(mIRQSocketAdapterIn[0]);
    mControl.FIQOutSocket[0].bind(mFIQSocketAdapterIn[0]);
    mControl.VIRQOutSocket[0].bind(mVIRQSocketAdapterIn[0]);
    mControl.VFIQOutSocket[0].bind(mVFIQSocketAdapterIn[0]);
    mControl.IRQOutSocket[1].bind(mIRQSocketAdapterIn[1]);
    mControl.FIQOutSocket[1].bind(mFIQSocketAdapterIn[1]);
    mControl.VIRQOutSocket[1].bind(mVIRQSocketAdapterIn[1]);
    mControl.VFIQOutSocket[1].bind(mVFIQSocketAdapterIn[1]);
    mControl.IRQOutSocket[2].bind(mIRQSocketAdapterIn[2]);
    mControl.FIQOutSocket[2].bind(mFIQSocketAdapterIn[2]);
    mControl.VIRQOutSocket[2].bind(mVIRQSocketAdapterIn[2]);
    mControl.VFIQOutSocket[2].bind(mVFIQSocketAdapterIn[2]);
    mControl.IRQOutSocket[3].bind(mIRQSocketAdapterIn[3]);
    mControl.FIQOutSocket[3].bind(mFIQSocketAdapterIn[3]);
    mControl.VIRQOutSocket[3].bind(mVIRQSocketAdapterIn[3]);
    mControl.VFIQOutSocket[3].bind(mVFIQSocketAdapterIn[3]);

    //** Exploding ARM Timer IRQ inputs **//
    mCPU.IRQOutSocket.bind(mARMCoreTimerAdapterIn);
    for (int cpuID = 0; cpuID < BCM2837_N_CPUS; ++cpuID) {
        mARMCoreTimerAdapterOut[cpuID][0].bind(mControl.CNTPNSIRQInSocket[cpuID]);
        mARMCoreTimerAdapterOut[cpuID][1].bind(mControl.CNTVIRQInSocket[cpuID]);
        mARMCoreTimerAdapterOut[cpuID][2].bind(mControl.CNTPSHPIRQInSocket[cpuID]);
        mARMCoreTimerAdapterOut[cpuID][3].bind(mControl.CNTPSIRQInSocket[cpuID]);
    }
    mARMCoreTimerAdapterIn.registerBTransport(this,
                                              &BCM2837<TLM_BUSWIDTH>::mARMTimerIRQInBTransport);

    mIRQSocketAdapterIn[0].registerBTransport(this,
                                              &BCM2837<TLM_BUSWIDTH>::template mIRQBTransport<0>);
    mIRQSocketAdapterIn[1].registerBTransport(this,
                                              &BCM2837<TLM_BUSWIDTH>::template mIRQBTransport<1>);
    mIRQSocketAdapterIn[2].registerBTransport(this,
                                              &BCM2837<TLM_BUSWIDTH>::template mIRQBTransport<2>);
    mIRQSocketAdapterIn[3].registerBTransport(this,
                                              &BCM2837<TLM_BUSWIDTH>::template mIRQBTransport<3>);
    mFIQSocketAdapterIn[0].registerBTransport(this,
                                              &BCM2837<TLM_BUSWIDTH>::template mFIQBTransport<0>);
    mFIQSocketAdapterIn[1].registerBTransport(this,
                                              &BCM2837<TLM_BUSWIDTH>::template mFIQBTransport<1>);
    mFIQSocketAdapterIn[2].registerBTransport(this,
                                              &BCM2837<TLM_BUSWIDTH>::template mFIQBTransport<2>);
    mFIQSocketAdapterIn[3].registerBTransport(this,
                                              &BCM2837<TLM_BUSWIDTH>::template mFIQBTransport<3>);
    mVIRQSocketAdapterIn[0].registerBTransport(this,
                                               &BCM2837<TLM_BUSWIDTH>::template mVIRQBTransport<0>);
    mVIRQSocketAdapterIn[1].registerBTransport(this,
                                               &BCM2837<TLM_BUSWIDTH>::template mVIRQBTransport<1>);
    mVIRQSocketAdapterIn[2].registerBTransport(this,
                                               &BCM2837<TLM_BUSWIDTH>::template mVIRQBTransport<2>);
    mVIRQSocketAdapterIn[3].registerBTransport(this,
                                               &BCM2837<TLM_BUSWIDTH>::template mVIRQBTransport<3>);
    mVFIQSocketAdapterIn[0].registerBTransport(this,
                                               &BCM2837<TLM_BUSWIDTH>::template mVFIQBTransport<0>);
    mVFIQSocketAdapterIn[1].registerBTransport(this,
                                               &BCM2837<TLM_BUSWIDTH>::template mVFIQBTransport<1>);
    mVFIQSocketAdapterIn[2].registerBTransport(this,
                                               &BCM2837<TLM_BUSWIDTH>::template mVFIQBTransport<2>);
    mVFIQSocketAdapterIn[3].registerBTransport(this,
                                               &BCM2837<TLM_BUSWIDTH>::template mVFIQBTransport<3>);

    //** CPU mapping **//
    mCPU.MMIOSocket.bind(mMemoryMappedRouter.inputSocket);
    mIRQSocketAdapterOut.bind(mCPU.IRQInSocket);

    //** ARMCtrl Clogs **//
    mARMCtrlClog.bind(mARMControl.ARMIRQIn);

    //** UART -> GPIO **//
    mGPIO.exposeUART0(mUART0.UARTSocket);

    //** Tweaks thread **//
    SC_THREAD(tweaksThread);

#ifndef NDEBUG
    std::cout << "[DEBUG] Listing parameters:" << std::endl;
    auto debugBroker(::cci::cci_get_broker());
    for (auto pDebug : ::cci::cci_get_broker().get_param_handles()) {
        std::cout << pDebug.name() << " = " << pDebug.get_cci_value() << std::endl;
    }
#endif
}

template <unsigned int TLM_BUSWIDTH> BCM2837<TLM_BUSWIDTH>::~BCM2837() {
}

template <unsigned int TLM_BUSWIDTH>
void BCM2837<TLM_BUSWIDTH>::mARMTimerIRQInBTransport(irq_payload_type &txn,
                                                     ::sc_core::sc_time &delay) {
    int id = txn.getID();

    mARMCoreTimerAdapterOut[id / 4][id % 4]->b_transport(txn, delay);

    if (txn.isResponseError()) {
        HV_ERR("Received error response");
    }
}

template <unsigned int TLM_BUSWIDTH> void BCM2837<TLM_BUSWIDTH>::tweaksThread() {
    // CPRMAN Tweak
    ::hv::common::hvuint8_t data[4] = {0xFF, 0xFF, 0xFF, 0xFF};
    ::hv::communication::tlm2::protocols::memorymapped::MemoryMappedPayload<::hv::common::hvaddr_t>
        txn;
    ::sc_core::sc_time zeroTime(::sc_core::SC_ZERO_TIME);

    // UART0 early connection tweak
    ::std::cout << "[GPIO tweak] Activating UART0 output on GPIO for early stdout" << ::std::endl;
    txn.setCommand(::hv::communication::tlm2::protocols::memorymapped::MEM_MAP_WRITE_COMMAND);
    txn.setAddress(0x3f20000c);
    txn.setDataLength(4);
    ::hv::common::BitVector GPIOTweak(32, 0);
    GPIOTweak(8, 6) = 7;
    GPIOTweak(11, 9) = 7;
    data[0] = GPIOTweak(7, 0);
    data[1] = GPIOTweak(15, 8);
    data[2] = GPIOTweak(23, 16);
    data[3] = GPIOTweak(31, 24);
    txn.setDataPtr(&data[0]);
    mCPU.MMIOSocket->b_transport(txn, zeroTime);
    HV_ASSERT(txn.isResponseOk(), "Response is not OK")
    while (1) {
        ::sc_core::wait(0.2, ::sc_core::SC_SEC);
    }
}

} // namespace hv
