/**
 * @file BCM2837.h
 * @author Benjamin Barrois <benjamin.barrois@hiventive.com>
 * @date Sep, 2018
 * @copyright Copyright (C) 2018, Hiventive.
 *
 * @brief Broadcom BCM2837
 */

#ifndef BCM2837_H
#define BCM2837_H

#define TARGET_AARCH64

#include <fstream>
#include <qmg2sc/qmg2sc.h>
#include <BCM2835-GPIO.h>
#include <BCM2835_armctrl_ic.h>
#include <BCM2836_control.h>
#include <PL011.h>
#include <memory.h>

#define BCM2837_N_CPUS 4
#define BCM2837_CPU_NAME "cortex-a53"

namespace hv {

template <unsigned int TLM_BUSWIDTH> class BCM2837 : public ::hv::module::Module {
  public:
    BCM2837(::hv::module::ModuleName name_);
    virtual ~BCM2837();

    // Note: GPIO is public as it is physically accessible from outside
    ::broadcom::BCM2835GPIO<TLM_BUSWIDTH> mGPIO;

    ::hv::cfg::Param<::hv::common::hvuint32_t> resetCBAR;
    ::hv::cfg::Param<::std::size_t> ramSize;
    ::hv::cfg::Param<::std::size_t> vcramSize;
    ::hv::cfg::Param<::hv::common::hvint32_t> boardId;
    ::hv::cfg::Param<::hv::common::hvaddr_t> smpBootAddr;
    ::hv::cfg::Param<::std::string> kernelPath;
    ::hv::cfg::Param<::std::string> kernelCmd;
    ::hv::cfg::Param<::std::string> initrdPath;
    ::hv::cfg::Param<::std::string> dtbPath;
    ::hv::cfg::Param<::std::string> sdPath;

    ::hv::cfg::Param<bool> activateGDBServer;
    ::hv::cfg::Param<::hv::common::hvuint16_t> gdbPort;

  protected:
    typedef ::hv::communication::tlm2::protocols::irq::IRQPayload irq_payload_type;
    typedef ::hv::communication::tlm2::protocols::uart::UartPayload uart_payload_type;
    typedef ::hv::communication::tlm2::protocols::signal::SignalPayload signal_payload_type;

    // CPU
    ::std::string cpuName;
    QMG2SC<TLM_BUSWIDTH> mCPU;

    // Memory-mapped router
    ::hv::communication::tlm2::protocols::memorymapped::MemoryMappedRouter<TLM_BUSWIDTH>
        mMemoryMappedRouter;

    // IPs
    ::arm::PL011<TLM_BUSWIDTH> mUART0;
    ::broadcom::BCM2836Control<TLM_BUSWIDTH> mControl;
    ::broadcom::BCM2835ARMctrlIC<TLM_BUSWIDTH> mARMControl;

    // SD Card
    QMGBus* mSDBus;
    QMGBus* mSDHCIBus;
    QMGBus* mSDHostBus;

    // CPRMAN Tweak
    // This is just a simple memory to retain value written
    ::hv::Memory<TLM_BUSWIDTH> mCPRMANTweak;

    // ARMControl clog
    ::hv::communication::tlm2::protocols::irq::IRQSimpleInitiatorSocket<> mARMCtrlClog;

    //** IRQ management **//
    QMGCPUDevice *cpuDevs[BCM2837_N_CPUS];
    QMGIRQ *timerOutputIRQs[4 * BCM2837_N_CPUS];
    QMGIRQ *cpuInputIRQs[4 * BCM2837_N_CPUS];
    ::hv::communication::tlm2::protocols::irq::IRQSimpleTargetSocket<
        ::hv::communication::tlm2::protocols::irq::IRQProtocolTypes, 0>
        mIRQSocketAdapterIn[4];
    ::hv::communication::tlm2::protocols::irq::IRQSimpleTargetSocket<
        ::hv::communication::tlm2::protocols::irq::IRQProtocolTypes, 0>
        mFIQSocketAdapterIn[4];
    ::hv::communication::tlm2::protocols::irq::IRQSimpleTargetSocket<
        ::hv::communication::tlm2::protocols::irq::IRQProtocolTypes, 0>
        mVIRQSocketAdapterIn[4];
    ::hv::communication::tlm2::protocols::irq::IRQSimpleTargetSocket<
        ::hv::communication::tlm2::protocols::irq::IRQProtocolTypes, 0>
        mVFIQSocketAdapterIn[4];

    ::hv::communication::tlm2::protocols::irq::IRQSimpleInitiatorSocket<> mIRQSocketAdapterOut;

    // ARM Timer
    ::hv::communication::tlm2::protocols::irq::IRQSimpleTargetSocket<> mARMCoreTimerAdapterIn;
    ::hv::communication::tlm2::protocols::irq::IRQSimpleInitiatorSocket<>
        mARMCoreTimerAdapterOut[4][4];

    template <unsigned int CORE_ID>
    void mIRQBTransport(irq_payload_type &txn, ::sc_core::sc_time &delay) {
        txn.setID(cpuInputIRQs[4 * CORE_ID]->IRQId);
        mIRQSocketAdapterOut->b_transport(txn, delay);
    }

    template <unsigned int CORE_ID>
    void mFIQBTransport(irq_payload_type &txn, ::sc_core::sc_time &delay) {
        txn.setID(cpuInputIRQs[4 * CORE_ID + 1]->IRQId);
        mIRQSocketAdapterOut->b_transport(txn, delay);
    }

    template <unsigned int CORE_ID>
    void mVIRQBTransport(irq_payload_type &txn, ::sc_core::sc_time &delay) {
        txn.setID(cpuInputIRQs[4 * CORE_ID + 2]->IRQId);
        mIRQSocketAdapterOut->b_transport(txn, delay);
    }

    template <unsigned int CORE_ID>
    void mVFIQBTransport(irq_payload_type &txn, ::sc_core::sc_time &delay) {
        txn.setID(cpuInputIRQs[4 * CORE_ID + 3]->IRQId);
        mIRQSocketAdapterOut->b_transport(txn, delay);
    }

    void mGPUIRQFIQInBTransport(irq_payload_type &txn, ::sc_core::sc_time &delay);
    void mARMTimerIRQInBTransport(irq_payload_type &txn, ::sc_core::sc_time &delay);

    void switchToSDHostCb(const bool &toSDHost);

    void tweaksThread();
};

} // namespace hv

#include "BCM2837.hpp"

#endif // BCM2837_H
