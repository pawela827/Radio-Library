#include "pxt.h"

// micro:bit dal
#if defined(MICROBIT_H) 

#define CODAL_RADIO MicroBitRadio
#define DEVICE_OK MICROBIT_OK
#define DEVICE_NOT_SUPPORTED MICROBIT_NOT_SUPPORTED
#define CODAL_EVENT MicroBitEvent
#define CODAL_RADIO_MICROBIT_DAL 1

// any other NRF52 board
#elif defined(NRF52_SERIES)

#include "NRF52Radio.h"
#define CODAL_RADIO codal::NRF52Radio
#define CODAL_EVENT codal::Event

#endif

using namespace pxt;

#ifndef MICROBIT_RADIO_MAX_PACKET_SIZE
#define MICROBIT_RADIO_MAX_PACKET_SIZE          32
#endif

#ifndef DEVICE_RADIO_MAX_PACKET_SIZE
#define DEVICE_RADIO_MAX_PACKET_SIZE MICROBIT_RADIO_MAX_PACKET_SIZE
#endif

#ifndef MICROBIT_ID_RADIO
#define MICROBIT_ID_RADIO               29
#endif

#ifndef DEVICE_ID_RADIO
#define DEVICE_ID_RADIO MICROBIT_ID_RADIO
#endif

#ifndef MICROBIT_RADIO_EVT_DATAGRAM
#define MICROBIT_RADIO_EVT_DATAGRAM             1       // Event to signal that a new datagram has been received.
#endif

#ifndef DEVICE_RADIO_EVT_DATAGRAM
#define DEVICE_RADIO_EVT_DATAGRAM MICROBIT_RADIO_EVT_DATAGRAM
#endif

// V2 (nRF52833) detection by chip, NOT by MICROBIT_H: codal-microbit-v2 also
// defines MICROBIT_H, so CODAL_RADIO_MICROBIT_DAL is 1 on V2 as well and can't
// be used to tell V1 and V2 apart.
#if defined(NRF52_SERIES) || defined(NRF52833_XXAA) || defined(NRF52)
#define RF_NRF52 1
#else
#define RF_NRF52 0
#endif

//% color=#E3008C weight=96 icon="\uf012"
namespace rf {
    
#if CODAL_RADIO_MICROBIT_DAL
    CODAL_RADIO* getRadio() {
        return &uBit.radio;
    }
#elif defined(CODAL_RADIO)
class RadioWrap {
    CODAL_RADIO radio;
    public:
        RadioWrap() 
            : radio()
        {}

    CODAL_RADIO* getRadio() {
        return &radio;
    }
};
SINGLETON(RadioWrap);
CODAL_RADIO* getRadio() {
    auto wrap = getRadioWrap();
    if (NULL != wrap)
        return wrap->getRadio();    
    return NULL;
}
#endif // #else

    bool radioEnabled = false;
    bool init = false;
    int radioEnable() {
#ifdef CODAL_RADIO
        auto radio = getRadio();
        if (NULL == radio) 
            return DEVICE_NOT_SUPPORTED;

        if (init && !radioEnabled) {
            //If radio was explicitly disabled from a call to off API
            //We don't want to enable it here. User needs to call on API first.
            return DEVICE_NOT_SUPPORTED;
        }

        int r = radio->enable();
        if (r != DEVICE_OK) {
            target_panic(43);
            return r;
        }
        if (!init) {
            getRadio()->setGroup(0); //Default group zero. This used to be pxt::programHash()
            getRadio()->setTransmitPower(6); // start with high power by default
            init = true;
        }
        radioEnabled = true;
        return r;
#else
        return DEVICE_NOT_SUPPORTED;
#endif
    }

    /**
    * Disables the radio for use as a multipoint sender/receiver.
    * Disabling radio will help conserve battery power when it is not in use.
    */
    //% help=rf/off
    void off() {
#ifdef CODAL_RADIO
        auto radio = getRadio();
        if (NULL == radio)
            return;

        int r = radio->disable();
        if (r != DEVICE_OK) {
            target_panic(43);
        } else {
            radioEnabled = false;
        }
#else
        return;
#endif
    }

    /**
    * Initialises the radio for use as a multipoint sender/receiver
    * Only useful when the radio.off() is used beforehand.
    */
    //% help=rf/on
    void on() {
#ifdef CODAL_RADIO
        auto radio = getRadio();
        if (NULL == radio)
            return;

        int r = radio->enable();
        if (r != DEVICE_OK) {
            target_panic(43);
        } else {
            radioEnabled = true;
        }
#else
        return;
#endif
    }

    /**
    * Sends an event over radio to neigboring devices
    */
    //% blockId=rfRaiseEvent block="rf raise event|from source %src=control_event_source_id|with value %value=control_event_value_id"
    //% src.label="source" value.label="value"
    //% blockExternalInputs=1
    //% advanced=true
    //% weight=1
    //% help=rf/raise-event
    void raiseEvent(int src, int value) {
#ifdef CODAL_RADIO        
        if (radioEnable() != DEVICE_OK) return;

        getRadio()->event.eventReceived(CODAL_EVENT(src, value, CREATE_ONLY));
#endif        
    }

    /**
     * Internal use only. Takes the next packet from the radio queue and returns its contents + RSSI in a Buffer.
     * @returns NULL if no packet available
     */
    //%
    Buffer readRawPacket() {
#ifdef CODAL_RADIO        
        if (radioEnable() != DEVICE_OK) return NULL;

        auto p = getRadio()->datagram.recv();
#if CODAL_RADIO_MICROBIT_DAL
        if (p == PacketBuffer::EmptyPacket)
            return NULL;
        int rssi = p.getRSSI();
        auto length = p.length();
        auto bytes = p.getBytes();
#else
        // TODO: RSSI support
        int rssi = -73;        
        auto length = p.length();
        auto bytes = p.getBytes();
        if (length == 0)
            return NULL;
#endif

        uint8_t buf[DEVICE_RADIO_MAX_PACKET_SIZE + sizeof(int)]; // packet length + rssi
        memset(buf, 0, sizeof(buf));
        memcpy(buf, bytes, length); // data
        memcpy(buf + DEVICE_RADIO_MAX_PACKET_SIZE, &rssi, sizeof(int)); // RSSi - assumes Int32LE layout
        return mkBuffer(buf, sizeof(buf));
#else
        return NULL;
#endif        
    }

    /**
     * Internal use only. Sends a raw packet through the radio (assumes RSSI appened to packet)
     */
    //% async
    void sendRawPacket(Buffer msg) {
#ifdef CODAL_RADIO        
        if (radioEnable() != DEVICE_OK || NULL == msg) return;

        // don't send RSSI data; and make sure no buffer underflow
        int len = msg->length - sizeof(int);
        if (len > 0)
            getRadio()->datagram.send(msg->data, len);
#endif            
    }

    /**
     * Used internally by the library.
     */
    //% help=rf/on-data-received
    //% weight=0
    //% blockId=rf_datagram_received_event block="rf on data received" blockGap=8
    //% deprecated=true blockHidden=1
    void onDataReceived(Action body) {
#ifdef CODAL_RADIO        
        if (radioEnable() != DEVICE_OK) return;

        registerWithDal(DEVICE_ID_RADIO, DEVICE_RADIO_EVT_DATAGRAM, body);
        getRadio()->datagram.recv(); // wake up read code
#endif       
    }

    /**
     * Sets the group id for radio communications. A micro:bit can only listen to one group ID at any time.
     * @param id the group id between ``0`` and ``255``, eg: 1
     */
    //% help=rf/set-group
    //% weight=100
    //% blockId=rf_set_group block="rf set group %ID"
    //% id.label="value"
    //% id.min=0 id.max=255
    //% group="Group"
    void setGroup(int id) {
#ifdef CODAL_RADIO        
        if (radioEnable() != DEVICE_OK) return;

        getRadio()->setGroup(id);
#endif       
    }

    /**
     * Change the output power level of the transmitter to the given value.
    * @param power a value in the range 0..7, where 0 is the lowest power and 7 is the highest. eg: 7
    */
    //% help=rf/set-transmit-power
    //% weight=9 blockGap=8
    //% blockId=rf_set_transmit_power block="rf set transmit power %power"
    //% power.label="value"
    //% power.min=0 power.max=7
    //% advanced=true
    void setTransmitPower(int power) {
#ifdef CODAL_RADIO        
        if (radioEnable() != DEVICE_OK) return;

        getRadio()->setTransmitPower(power);
#endif        
    }

#if RF_NRF52
    // ---------- protocol-level raw antenna access (V2 / nRF52833 only) ----------
    // Everything below talks to the RADIO peripheral directly, bypassing the
    // MicroBitRadio/NRF52Radio protocol layer entirely (its fixed BASE0="uBit"
    // address match, CRC and whitening). A "protocol" here is just a name for
    // one particular combination of CRCCNF/PCNF0/PCNF1/DATAWHITEIV/MODE - the
    // registers that decide what counts as a valid packet on the air. Adding a
    // new protocol later means adding one more case below with its own register
    // values; rf.scanRaw()/readRawAntennaPacket() and rf.sendRawPacket() don't
    // need to change, since they just move bytes in and out of rawRxBuf either way.

    // keep in sync with RFProtocol in radio.ts/shims.d.ts
    const int PROTOCOL_MAKECODE = 0; // normal micro:bit packets (MicroBitRadio/NRF52Radio defaults)
    const int PROTOCOL_RAW = 1;      // promiscuous: no address match, no CRC, no whitening
    const int PROTOCOL_ESB = 2;      // Nordic (Enhanced) ShockBurst - compatible with nRF24L01(+)
    const int PROTOCOL_GAZELL = 5;   // Nordic Gazell - see the NRF_GZLL_AVAILABLE block below
    // reserved for later: PROTOCOL_ZIGBEE = 3, PROTOCOL_BLE = 4

    int currentProtocol = PROTOCOL_MAKECODE;

    // registers saved before leaving PROTOCOL_MAKECODE, so switching back
    // restores normal rf.on()/send()/recv() behaviour exactly as it was
    uint32_t savedCRCCNF, savedPCNF0, savedPCNF1, savedDATAWHITEIV, savedSHORTS, savedMODE;
    bool savedRegistersValid = false;
    // +2: large enough for ESB's [S0][S1][payload...] layout as well as
    // PROTOCOL_RAW's plain [payload...] layout - see currentCaptureLength()
    uint8_t rawRxBuf[DEVICE_RADIO_MAX_PACKET_SIZE + 2];

    // how many bytes of rawRxBuf actually hold real data for the active
    // protocol - PROTOCOL_RAW has no header (PCNF0=0), PROTOCOL_ESB has a
    // 2-byte [S0][S1] header in front of the payload (see enterEsbProtocol)
    int currentCaptureLength() {
        switch (currentProtocol) {
            case PROTOCOL_ESB: return DEVICE_RADIO_MAX_PACKET_SIZE + 2;
            default: return DEVICE_RADIO_MAX_PACKET_SIZE;
        }
    }

    // ---------- ESB (Enhanced ShockBurst / nRF24L01-compatible) ----------
    // Register values below follow Nordic's own reference implementation
    // (nrf51-micro-esb), using the legacy ShockBurst framing: static 32-byte
    // payload, no dynamic-payload-length byte, big-endian on air, 2-byte CRC
    // with the same CRCINIT/CRCPOLY as Bluetooth's default CRC-16. This is the
    // framing nRF24L01(+) modules and most cheap ShockBurst peripherals
    // (mice, remotes, toys) actually speak, and is simpler than the newer
    // dynamic-payload-length (ESB_DPL) variant.
    uint8_t esbAddress[5] = { 0xE7, 0xE7, 0xE7, 0xE7, 0xE7 }; // common default, eg. many RF24 libraries

    // ShockBurst addresses are transmitted MSB-first per byte, but the RADIO
    // peripheral's BASE/PREFIX registers store them bit-reversed per byte -
    // same helper as Nordic's own esb library (bytewise_bit_swap)
    uint32_t esbBitSwap(uint32_t inp) {
        inp = (inp & 0xF0F0F0F0) >> 4 | (inp & 0x0F0F0F0F) << 4;
        inp = (inp & 0xCCCCCCCC) >> 2 | (inp & 0x33333333) << 2;
        inp = (inp & 0xAAAAAAAA) >> 1 | (inp & 0x55555555) << 1;
        return inp;
    }

    void applyEsbAddress() {
        // PREFIX0 holds the address's first (most significant) byte,
        // BASE0 holds the remaining 4 bytes - both bit-reversed per byte
        NRF_RADIO->PREFIX0 = esbBitSwap((uint32_t)esbAddress[0]);
        NRF_RADIO->BASE0 = esbBitSwap(
            ((uint32_t)esbAddress[1] << 24) | ((uint32_t)esbAddress[2] << 16) |
            ((uint32_t)esbAddress[3] << 8) | (uint32_t)esbAddress[4]);
    }

    void enterEsbProtocol() {
        NRF_RADIO->TASKS_DISABLE = 1;
        while (NRF_RADIO->EVENTS_DISABLED == 0) {}
        NRF_RADIO->EVENTS_DISABLED = 0;

        NRF_RADIO->MODE = RADIO_MODE_MODE_Nrf_1Mbit << RADIO_MODE_MODE_Pos; // matches nRF24L01(+) default rate

        // legacy ShockBurst framing: S0=1 byte, no length field, S1=1 byte (PCF)
        NRF_RADIO->PCNF0 = (1 << RADIO_PCNF0_S0LEN_Pos)
            | (0 << RADIO_PCNF0_LFLEN_Pos)
            | (1 << RADIO_PCNF0_S1LEN_Pos);
        NRF_RADIO->PCNF1 = (RADIO_PCNF1_WHITEEN_Disabled << RADIO_PCNF1_WHITEEN_Pos)
            | (RADIO_PCNF1_ENDIAN_Big << RADIO_PCNF1_ENDIAN_Pos)     // ESB is big-endian on air
            | (4 << RADIO_PCNF1_BALEN_Pos)                            // base address length: 4 bytes (+1 prefix byte = 5)
            | ((uint32_t)(DEVICE_RADIO_MAX_PACKET_SIZE) << RADIO_PCNF1_STATLEN_Pos) // static 32-byte payload
            | ((uint32_t)(DEVICE_RADIO_MAX_PACKET_SIZE) << RADIO_PCNF1_MAXLEN_Pos);

        NRF_RADIO->CRCCNF = RADIO_CRCCNF_LEN_Two << RADIO_CRCCNF_LEN_Pos; // 2-byte CRC, like nRF24L01(+) default
        NRF_RADIO->CRCINIT = 0xFFFFUL;
        NRF_RADIO->CRCPOLY = 0x11021UL;

        applyEsbAddress();
        NRF_RADIO->TXADDRESS = 0;
        NRF_RADIO->RXADDRESSES = 1; // listen on logical address 0 (PREFIX0/BASE0) only

        // keep only READY->START (auto-arm on enable) and ADDRESS->RSSISTART (RSSI per capture)
        NRF_RADIO->SHORTS = (1 << RADIO_SHORTS_READY_START_Pos)
            | (1 << RADIO_SHORTS_ADDRESS_RSSISTART_Pos);

        NRF_RADIO->PACKETPTR = (uint32_t)rawRxBuf;
        NRF_RADIO->TASKS_RXEN = 1;
        while (NRF_RADIO->EVENTS_READY == 0) {}
        NRF_RADIO->EVENTS_READY = 0;
        NRF_RADIO->TASKS_START = 1;
    }

    void enterRawProtocol() {
        NRF_RADIO->TASKS_DISABLE = 1;
        while (NRF_RADIO->EVENTS_DISABLED == 0) {}
        NRF_RADIO->EVENTS_DISABLED = 0;

        NRF_RADIO->MODE = RADIO_MODE_MODE_Nrf_1Mbit << RADIO_MODE_MODE_Pos; // 1Mbit catches the widest range of GFSK gear
        NRF_RADIO->CRCCNF = RADIO_CRCCNF_LEN_Disabled; // don't drop "invalid" CRC packets
        // treat everything as one long raw blob: 0-bit length field, max-size static payload
        NRF_RADIO->PCNF0 = 0;
        NRF_RADIO->PCNF1 = (uint32_t)(DEVICE_RADIO_MAX_PACKET_SIZE)
            | (RADIO_PCNF1_ENDIAN_Little << RADIO_PCNF1_ENDIAN_Pos)
            | (0 << RADIO_PCNF1_WHITEEN_Pos); // whitening off - capture the raw air bytes
        // keep only READY->START (auto-arm on enable) and ADDRESS->RSSISTART (RSSI per capture)
        NRF_RADIO->SHORTS = (1 << RADIO_SHORTS_READY_START_Pos)
            | (1 << RADIO_SHORTS_ADDRESS_RSSISTART_Pos);

        NRF_RADIO->PACKETPTR = (uint32_t)rawRxBuf;
        NRF_RADIO->TASKS_RXEN = 1;
        while (NRF_RADIO->EVENTS_READY == 0) {}
        NRF_RADIO->EVENTS_READY = 0;
        NRF_RADIO->TASKS_START = 1;
    }

    void enterMakeCodeProtocol() {
        NRF_RADIO->TASKS_DISABLE = 1;
        while (NRF_RADIO->EVENTS_DISABLED == 0) {}
        NRF_RADIO->EVENTS_DISABLED = 0;

        if (savedRegistersValid) {
            NRF_RADIO->CRCCNF = savedCRCCNF;
            NRF_RADIO->PCNF0 = savedPCNF0;
            NRF_RADIO->PCNF1 = savedPCNF1;
            NRF_RADIO->DATAWHITEIV = savedDATAWHITEIV;
            NRF_RADIO->SHORTS = savedSHORTS;
            NRF_RADIO->MODE = savedMODE;
        }

        // hand control back to the normal protocol layer (re-does RXEN/START
        // with its own configuration, exactly as MicroBitRadio::enable() would)
        getRadio()->disable();
        getRadio()->enable();
    }

    // ---------- Gazell (PROTOCOL_GAZELL) - NOT WIRED UP YET ----------
    // Unlike Raw/Esb above, Gazell isn't something we can drive by writing a
    // few RADIO registers ourselves: nrf_gzll is Nordic's own closed-source
    // link-layer library (prebuilt .a per chip/toolchain, no public source),
    // and once enabled it takes over RADIO_IRQHandler and a hardware TIMER
    // itself, running its own frequency-hopping state machine and calling
    // your code back through callbacks - it doesn't hand bytes back through
    // a register/buffer we can just peek at like PROTOCOL_RAW/PROTOCOL_ESB do.
    // So this can't be "one more case in the switch" the way ESB was.
    //
    // What real integration needs, in order:
    //   1. Get the library into this extension: download the nRF5 SDK
    //      (nordicsemi.com/Products/Development-software/nRF5-SDK), and from
    //      components/proprietary_rf/gzll/ take nrf_gzll.h, nrf_gzll_constants.h,
    //      plus the prebuilt lib for nRF52833 (an ARM Cortex-M4/soft-float
    //      variant - matching MakeCode's own build flags matters here) from
    //      lib/. Add the .h files to this extension and the .a as a static lib
    //      MakeCode's build links against (needs a pxt.json / yotta-style
    //      addition most MakeCode extensions don't otherwise need).
    //   2. Implement the callbacks nrf_gzll.h declares as extern "C" hooks -
    //      at minimum nrf_gzll_host_rx_data_ready() (called from the library's
    //      own IRQ context when a packet arrives) and nrf_gzll_disabled() -
    //      and have them push into rawRxBuf/a small queue the same way
    //      readRawAntennaPacket() expects, OR give Gazell its own read function
    //      instead of reusing readRawAntennaPacket() (it doesn't share
    //      Raw/Esb's PACKETPTR-based capture at all).
    //   3. In enterGazellProtocol() (to replace this stub): call
    //      nrf_gzll_init(NRF_GZLL_MODE_HOST), configure channel table /
    //      addresses / datarate via its setters, then nrf_gzll_enable(). In
    //      the MakeCode-protocol case, call nrf_gzll_disable() and wait for
    //      nrf_gzll_disabled() before handing RADIO back to getRadio()->enable()
    //      - Gazell must be cleanly disabled before anything else touches RADIO.
    //   4. setFrequencyBand()/setGroup() as written don't apply to Gazell (it
    //      manages its own channel table and pipe addresses) - decide whether
    //      those should be redirected to nrf_gzll_set_channel_table()/
    //      nrf_gzll_set_base_address_0() while this protocol is active, or
    //      simply ignored with that documented.
    //
    // None of steps 1-2 can be done from here: they need the actual Nordic SDK
    // binary, which has to be downloaded and license-accepted outside this
    // session. Once you have it, this comment block is the checklist; ping me
    // and we'll write enterGazellProtocol() and the callbacks against the real
    // header.
#ifdef NRF_GZLL_AVAILABLE
    void enterGazellProtocol() {
        // TODO: nrf_gzll_init(NRF_GZLL_MODE_HOST); configure channel table,
        // addresses, datarate; nrf_gzll_enable(); see checklist above.
    }
#endif

    uint8_t rawTxBuf[DEVICE_RADIO_MAX_PACKET_SIZE + 2]; // same layout as rawRxBuf - see currentCaptureLength()
#endif // RF_NRF52

    // ---------- exported functions ----------
    // Each one is defined exactly once (so MakeCode's shim parser sees it once),
    // with the V2-only body inside #if RF_NRF52 and a harmless stub for V1.

    /**
     * Sets the 5-byte on-air address ESB listens to and sends with - the same
     * role as the address configured on an nRF24L01(+) module. Only takes
     * effect while rf.setProtocol(RFProtocol.Esb) is active; call it again
     * after switching protocol if you need a non-default address.
     * @param address exactly 5 bytes, eg: hex literal like E7E7E7E7E7
     */
    //% help=rf/set-esb-address
    //% weight=5 blockGap=8
    //% blockId=rf_set_esb_address block="rf set esb address %address"
    //% advanced=true
    void setEsbAddress(Buffer address) {
#if RF_NRF52
        if (NULL == address || address->length != 5) return;
        for (int i = 0; i < 5; i++)
            esbAddress[i] = address->data[i];
        if (currentProtocol == PROTOCOL_ESB)
            applyEsbAddress();
#else
        (void)address;
#endif
    }


    /**
     * Switches the radio to a different protocol/framing. Each protocol is a
     * different combination of address matching, CRC and whitening on the same
     * RADIO peripheral - switching is instant and doesn't need re-flashing.
     * rf.setFrequencyBand() and rf.setGroup()/setTransmitPower() keep working
     * the same way regardless of which protocol is active.
     * @param protocol which protocol to switch to, eg: RFProtocol.MakeCode
     */
    //% help=rf/set-protocol
    //% weight=7 blockGap=8
    //% blockId=rf_set_protocol block="rf set protocol %protocol"
    //% advanced=true
    void setProtocol(int protocol) {
#if RF_NRF52
        if (radioEnable() != DEVICE_OK) return;
        if (protocol == currentProtocol) return;

        // leaving PROTOCOL_MAKECODE for the first time: remember its registers
        if (currentProtocol == PROTOCOL_MAKECODE && !savedRegistersValid) {
            savedCRCCNF = NRF_RADIO->CRCCNF;
            savedPCNF0 = NRF_RADIO->PCNF0;
            savedPCNF1 = NRF_RADIO->PCNF1;
            savedDATAWHITEIV = NRF_RADIO->DATAWHITEIV;
            savedSHORTS = NRF_RADIO->SHORTS;
            savedMODE = NRF_RADIO->MODE;
            savedRegistersValid = true;
        }

        switch (protocol) {
            case PROTOCOL_MAKECODE:
                enterMakeCodeProtocol();
                break;
            case PROTOCOL_RAW:
                enterRawProtocol();
                break;
            case PROTOCOL_ESB:
                enterEsbProtocol();
                break;
            case PROTOCOL_GAZELL:
#ifdef NRF_GZLL_AVAILABLE
                enterGazellProtocol();
                break;
#else
                // the nrf_gzll library isn't linked into this build yet -
                // see the checklist above enterGazellProtocol(). Ignore the
                // request rather than silently doing nothing useful.
                return;
#endif
            default:
                // unknown protocol - ignore the request, stay on the current one
                return;
        }

        currentProtocol = protocol;
#else
        (void)protocol;
#endif
    }

    /**
     * Which protocol the radio is currently using.
     */
    //% help=rf/get-protocol
    //% weight=6 blockGap=8
    //% advanced=true
    int getProtocol() {
#if RF_NRF52
        return currentProtocol;
#else
        return 0;
#endif
    }

    /**
     * Measures the current energy on the antenna at the active channel, in dBm,
     * independently of whether any recognisable packet is present. This is the
     * same "spectrum scanner" style reading other 2.4GHz radios expose - it
     * does not require a particular protocol and does not decode anything.
     * @returns signal strength in dBm (negative; closer to 0 = stronger), or 0 if unavailable
     */
    //% help=rf/scan-rssi
    //% weight=10 blockGap=8
    //% blockId=rf_scan_rssi block="rf scan rssi"
    //% advanced=true
    int scanRSSI() {
#if RF_NRF52
        if (radioEnable() != DEVICE_OK) return 0;

        bool wasReceiving = (NRF_RADIO->STATE == RADIO_STATE_STATE_Rx);
        if (!wasReceiving) {
            NRF_RADIO->PACKETPTR = (uint32_t)rawRxBuf;
            NRF_RADIO->TASKS_RXEN = 1;
            while (NRF_RADIO->EVENTS_READY == 0) {}
            NRF_RADIO->EVENTS_READY = 0;
        }

        NRF_RADIO->EVENTS_RSSIEND = 0;
        NRF_RADIO->TASKS_RSSISTART = 1;
        while (NRF_RADIO->EVENTS_RSSIEND == 0) {}
        NRF_RADIO->EVENTS_RSSIEND = 0;

        int rssi = -(int)(NRF_RADIO->RSSISAMPLE);
        NRF_RADIO->TASKS_RSSISTOP = 1;

        return rssi;
#else
        return 0;
#endif
    }

    /**
     * Internal use only. While the radio is on any protocol other than
     * MakeCode (Raw, Esb, ...), returns whatever bytes were last captured off
     * the air on the current channel using that protocol's framing, together
     * with their RSSI. On Esb this means: only packets matching the address
     * set with rf.setEsbAddress() and passing its CRC.
     * @returns NULL if on PROTOCOL_MAKECODE or nothing captured yet
     */
    //%
    Buffer readRawAntennaPacket() {
#if RF_NRF52
        if (currentProtocol == PROTOCOL_MAKECODE) return NULL;
        if (NRF_RADIO->EVENTS_END == 0) return NULL;
        NRF_RADIO->EVENTS_END = 0;

        int rssi = -(int)(NRF_RADIO->RSSISAMPLE);
        int length = currentCaptureLength(); // depends on the active protocol's framing

        uint8_t buf[DEVICE_RADIO_MAX_PACKET_SIZE + 2 + sizeof(int)]; // captured bytes + rssi
        memset(buf, 0, sizeof(buf));
        memcpy(buf, rawRxBuf, length);
        memcpy(buf + length, &rssi, sizeof(int));

        // radio keeps listening automatically (SHORTS: READY->START on re-enable elsewhere);
        // re-arm reception for the next capture
        NRF_RADIO->PACKETPTR = (uint32_t)rawRxBuf;
        NRF_RADIO->TASKS_START = 1;

        return mkBuffer(buf, length + sizeof(int));
#else
        return NULL;
#endif
    }

    /**
     * Sends raw bytes on-air using the given protocol's framing - one function
     * for every raw-capable protocol instead of a separate send function per
     * protocol, since they all share the same TXEN/PACKETPTR/END sequence and
     * only differ in which registers setProtocol() already configured (this
     * keeps extra flash usage down, which matters on a 512KB part). Switches
     * to that protocol first if the radio isn't already on it (same one-call
     * convenience as rf.setRFProtocol() + rf.scanRaw(), but in one step).
     * The exact bytes expected depend on the protocol:
     *  - Raw: up to 32 bytes, sent exactly as given (no header)
     *  - Esb: up to 32 bytes; passed through as the payload, with the [S0][S1]
     *    header bytes rf.scanRaw() exposes as esbS0/esbS1 both set to 0
     * MakeCode isn't accepted here - use rf.sendNumber()/sendString()/etc
     * instead, which speak the normal micro:bit packet format.
     * @param protocol which protocol to send with, eg: RFProtocol.Esb
     * @param data the bytes to transmit
     */
    //% help=rf/send-raw-antenna-packet
    //% weight=4 blockGap=8
    //% blockId=rf_send_raw_antenna_packet block="rf send raw %protocol packet %data"
    //% advanced=true
    void sendRawAntennaPacket(int protocol, Buffer data) {
#if RF_NRF52
        if (protocol == PROTOCOL_MAKECODE || protocol == PROTOCOL_GAZELL) return;
        if (NULL == data || data->length == 0) return;

        if (protocol != currentProtocol)
            setProtocol(protocol);
        if (protocol != currentProtocol) return; // setProtocol rejected it (eg. unknown/unavailable)

        int length = currentCaptureLength();
        int headerLen = length - DEVICE_RADIO_MAX_PACKET_SIZE; // 0 for Raw, 2 for Esb ([S0][S1])
        int payloadLen = data->length;
        if (payloadLen > DEVICE_RADIO_MAX_PACKET_SIZE) payloadLen = DEVICE_RADIO_MAX_PACKET_SIZE;

        memset(rawTxBuf, 0, sizeof(rawTxBuf));
        memcpy(rawTxBuf + headerLen, data->data, payloadLen); // S0/S1 (if any) stay 0

        // radio is currently RX-armed (setProtocol/enter*Protocol left it
        // listening) - stop, send, then re-arm for RX so scanRaw() keeps working
        NRF_RADIO->TASKS_DISABLE = 1;
        while (NRF_RADIO->EVENTS_DISABLED == 0) {}
        NRF_RADIO->EVENTS_DISABLED = 0;

        NRF_RADIO->PACKETPTR = (uint32_t)rawTxBuf;
        NRF_RADIO->EVENTS_END = 0;
        NRF_RADIO->TASKS_TXEN = 1;
        while (NRF_RADIO->EVENTS_READY == 0) {}
        NRF_RADIO->EVENTS_READY = 0;
        NRF_RADIO->TASKS_START = 1;
        while (NRF_RADIO->EVENTS_END == 0) {}
        NRF_RADIO->EVENTS_END = 0;

        // back to RX so rf.scanRaw()/readRawAntennaPacket() keep receiving
        NRF_RADIO->PACKETPTR = (uint32_t)rawRxBuf;
        NRF_RADIO->TASKS_RXEN = 1;
        while (NRF_RADIO->EVENTS_READY == 0) {}
        NRF_RADIO->EVENTS_READY = 0;
        NRF_RADIO->TASKS_START = 1;
#else
        (void)protocol; (void)data;
#endif
    }

    /**
    * Change the transmission and reception band of the radio to the given channel.
    * A single continuous parameter spanning the chip's full RF range: 0 = 2360MHz, 140 = 2500MHz.
    * Internally this is split across the nRF52833 RADIO peripheral's two frequency maps:
    * band 0-99 selects MAP=Low (2360-2459MHz), band 100-140 selects MAP=Default (2460-2500MHz).
    * @param band a frequency band in the range 0 - 140. Each step is 1MHz wide, based at 2360MHz.
    **/
    //% help=rf/set-frequency-band
    //% weight=8 blockGap=8
    //% blockId=rf_set_frequency_band block="rf set frequency band %band"
    //% band.label="value"
    //% band.min=0 band.max=140
    //% advanced=true
    void setFrequencyBand(int band) {
#ifdef CODAL_RADIO
        if (radioEnable() != DEVICE_OK) return;

        if (band < 0 || band > 140) return;

#if !RF_NRF52
        // micro:bit V1 (nRF51822): no MAP register, hardware only spans 2400-2500MHz.
        // Clamp into the DAL's native 0-100 range (2400-2500MHz); values below 40
        // (i.e. below 2400MHz on the V2 scale) are not reachable on V1.
        int v1Band = band - 40;
        if (v1Band < 0) v1Band = 0;
        if (v1Band > 100) v1Band = 100;
        getRadio()->setFrequencyBand(v1Band);
#else
        // micro:bit V2 (nRF52833): use the RADIO peripheral's MAP bit directly, since
        // CODAL's setFrequencyBand() only ever writes FREQUENCY and leaves MAP at
        // its power-on default (Default = 2400-2500MHz), so the 2360-2459MHz half of
        // the chip's documented operating range (2360-2500MHz) is otherwise unreachable.
        // on nRF52833 MAP is not a separate register - it is bit 8 of FREQUENCY
        if (band < 100) {
            // 0-99 -> MAP=Low -> channel = 2360 + FREQUENCY (FREQUENCY 0-99)
            NRF_RADIO->FREQUENCY = (RADIO_FREQUENCY_MAP_Low << RADIO_FREQUENCY_MAP_Pos)
                | (uint32_t)band;
        } else {
            // 100-140 -> MAP=Default -> channel = 2400 + FREQUENCY (FREQUENCY 60-100)
            NRF_RADIO->FREQUENCY = (RADIO_FREQUENCY_MAP_Default << RADIO_FREQUENCY_MAP_Pos)
                | (uint32_t)(band - 40);
        }
#endif
#endif
    }
}
