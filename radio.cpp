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
    //% help=radio/off
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
    //% help=radio/on
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
    //% blockId=radioRaiseEvent block="radio raise event|from source %src=control_event_source_id|with value %value=control_event_value_id"
    //% src.label="source" value.label="value"
    //% blockExternalInputs=1
    //% advanced=true
    //% weight=1
    //% help=radio/raise-event
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
    //% help=radio/on-data-received
    //% weight=0
    //% blockId=radio_datagram_received_event block="radio on data received" blockGap=8
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
    //% help=radio/set-group
    //% weight=100
    //% blockId=radio_set_group block="radio set group %ID"
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
    //% help=radio/set-transmit-power
    //% weight=9 blockGap=8
    //% blockId=radio_set_transmit_power block="radio set transmit power %power"
    //% power.label="value"
    //% power.min=0 power.max=7
    //% advanced=true
    void setTransmitPower(int power) {
#ifdef CODAL_RADIO        
        if (radioEnable() != DEVICE_OK) return;

        getRadio()->setTransmitPower(power);
#endif        
    }

#if defined(NRF52_SERIES) && !CODAL_RADIO_MICROBIT_DAL
    // ---------- raw antenna access (V2 / nRF52833 only) ----------
    // Everything below talks to the RADIO peripheral directly, bypassing the
    // MicroBitRadio/NRF52Radio protocol layer entirely (its fixed BASE0="uBit"
    // address match, CRC and whitening). This is how a real "raw scan" has to
    // work: the protocol layer only ever hands back bytes that already passed
    // its own framing, so anything off-protocol never reaches it as data.

    bool promiscuous = false;

    // registers saved before entering promiscuous mode, so normal rf.on()/
    // send()/recv() keeps working correctly once promiscuous mode is turned off
    uint32_t savedCRCCNF, savedPCNF0, savedPCNF1, savedDATAWHITEIV, savedSHORTS;
    uint8_t rawRxBuf[DEVICE_RADIO_MAX_PACKET_SIZE + 2]; // +2: raw packets carry their own length byte(s)

    /**
     * Turns raw "monitor mode" on or off. While on, the radio stops enforcing
     * the micro:bit packet framing (address match, whitening, CRC) so
     * rf.readRawAntennaPacket() can see whatever is actually on the air on the
     * current channel - not just valid micro:bit packets. Normal rf.on()/send()/
     * receive still work as before once this is turned back off.
     * @param enabled true to start sniffing raw bytes, false to return to normal mode
     */
    //% help=radio/set-promiscuous-mode
    //% weight=7 blockGap=8
    //% blockId=radio_set_promiscuous_mode block="rf set promiscuous mode %enabled"
    //% advanced=true
    void setPromiscuousMode(bool enabled) {
        if (radioEnable() != DEVICE_OK) return;
        if (enabled == promiscuous) return;

        if (enabled) {
            // save what MicroBitRadio/NRF52Radio configured, so we can restore it
            savedCRCCNF = NRF_RADIO->CRCCNF;
            savedPCNF0 = NRF_RADIO->PCNF0;
            savedPCNF1 = NRF_RADIO->PCNF1;
            savedDATAWHITEIV = NRF_RADIO->DATAWHITEIV;
            savedSHORTS = NRF_RADIO->SHORTS;

            NRF_RADIO->TASKS_DISABLE = 1;
            while (NRF_RADIO->EVENTS_DISABLED == 0) {}
            NRF_RADIO->EVENTS_DISABLED = 0;

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

            promiscuous = true;
        } else {
            NRF_RADIO->TASKS_DISABLE = 1;
            while (NRF_RADIO->EVENTS_DISABLED == 0) {}
            NRF_RADIO->EVENTS_DISABLED = 0;

            NRF_RADIO->CRCCNF = savedCRCCNF;
            NRF_RADIO->PCNF0 = savedPCNF0;
            NRF_RADIO->PCNF1 = savedPCNF1;
            NRF_RADIO->DATAWHITEIV = savedDATAWHITEIV;
            NRF_RADIO->SHORTS = savedSHORTS;

            promiscuous = false;

            // hand control back to the normal protocol layer (re-does RXEN/START
            // with its own configuration, exactly as MicroBitRadio::enable() would)
            getRadio()->disable();
            getRadio()->enable();
        }
    }

    /**
     * Whether raw promiscuous/monitor mode is currently on.
     */
    //% help=radio/is-promiscuous-mode
    //% weight=6 blockGap=8
    //% advanced=true
    bool isPromiscuousMode() {
        return promiscuous;
    }

    /**
     * Measures the current energy on the antenna at the active channel, in dBm,
     * independently of whether any recognisable packet is present. This is the
     * same "spectrum scanner" style reading other 2.4GHz radios expose - it
     * does not require promiscuous mode and does not decode anything.
     * @returns signal strength in dBm (negative; closer to 0 = stronger), or 0 if unavailable
     */
    //% help=radio/scan-rssi
    //% weight=10 blockGap=8
    //% blockId=radio_scan_rssi block="rf scan rssi"
    //% advanced=true
    int scanRSSI() {
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
    }

    /**
     * Internal use only. While promiscuous mode is on, returns whatever raw
     * bytes were last captured off the air on the current channel, together
     * with their RSSI - regardless of whether they form a valid micro:bit
     * packet. Call rf.setPromiscuousMode(true) first.
     * @returns NULL if promiscuous mode is off or nothing has been captured yet
     */
    //%
    Buffer readRawAntennaPacket() {
        if (!promiscuous) return NULL;
        if (NRF_RADIO->EVENTS_END == 0) return NULL;
        NRF_RADIO->EVENTS_END = 0;

        int rssi = -(int)(NRF_RADIO->RSSISAMPLE);
        int length = DEVICE_RADIO_MAX_PACKET_SIZE; // fixed-size raw capture (see PCNF0/PCNF1 above)

        uint8_t buf[DEVICE_RADIO_MAX_PACKET_SIZE + sizeof(int)]; // raw bytes + rssi
        memset(buf, 0, sizeof(buf));
        memcpy(buf, rawRxBuf, length);
        memcpy(buf + DEVICE_RADIO_MAX_PACKET_SIZE, &rssi, sizeof(int));

        // radio keeps listening automatically (SHORTS: READY->START on re-enable elsewhere);
        // re-arm reception for the next capture
        NRF_RADIO->PACKETPTR = (uint32_t)rawRxBuf;
        NRF_RADIO->TASKS_START = 1;

        return mkBuffer(buf, sizeof(buf));
    }
#endif // NRF52_SERIES && !DAL

    /**
    * Change the transmission and reception band of the radio to the given channel.
    * A single continuous parameter spanning the chip's full RF range: 0 = 2360MHz, 140 = 2500MHz.
    * Internally this is split across the nRF52833 RADIO peripheral's two frequency maps:
    * band 0-99 selects MAP=Low (2360-2459MHz), band 100-140 selects MAP=Default (2460-2500MHz).
    * @param band a frequency band in the range 0 - 140. Each step is 1MHz wide, based at 2360MHz.
    **/
    //% help=radio/set-frequency-band
    //% weight=8 blockGap=8
    //% blockId=radio_set_frequency_band block="radio set frequency band %band"
    //% band.label="value"
    //% band.min=0 band.max=140
    //% advanced=true
    void setFrequencyBand(int band) {
#ifdef CODAL_RADIO
        if (radioEnable() != DEVICE_OK) return;

        if (band < 0 || band > 140) return;

#if CODAL_RADIO_MICROBIT_DAL
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
        if (band < 100) {
            // 0-99 -> MAP=Low -> channel = 2360 + FREQUENCY (FREQUENCY 0-99)
            NRF_RADIO->MAP = (RADIO_MAP_MAP_Low << RADIO_MAP_MAP_Pos);
            NRF_RADIO->FREQUENCY = (uint32_t)band;
        } else {
            // 100-140 -> MAP=Default -> channel = 2400 + FREQUENCY (FREQUENCY 60-100)
            NRF_RADIO->MAP = (RADIO_MAP_MAP_Default << RADIO_MAP_MAP_Pos);
            NRF_RADIO->FREQUENCY = (uint32_t)(band - 40);
        }
#endif
#endif
    }
}
