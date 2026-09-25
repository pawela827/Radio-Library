// Auto-generated. Do not edit.



    //% color=#E3008C weight=96 icon="\uf012"
declare namespace rf {

    /**
     * Disables the radio for use as a multipoint sender/receiver.
     * Disabling radio will help conserve battery power when it is not in use.
     */
    //% help=rf/off shim=rf::off
    function off(): void;

    /**
     * Initialises the radio for use as a multipoint sender/receiver
     * Only useful when the radio.off() is used beforehand.
     */
    //% help=rf/on shim=rf::on
    function on(): void;

    /**
     * Sends an event over radio to neigboring devices
     */
    //% blockId=rfRaiseEvent block="rf raise event|from source %src=control_event_source_id|with value %value=control_event_value_id"
    //% src.label="source" value.label="value"
    //% blockExternalInputs=1
    //% advanced=true
    //% weight=1
    //% help=rf/raise-event shim=rf::raiseEvent
    function raiseEvent(src: int32, value: int32): void;

    /**
     * Internal use only. Takes the next packet from the radio queue and returns its contents + RSSI in a Buffer.
     * @returns NULL if no packet available
     */
    //% shim=rf::readRawPacket
    function readRawPacket(): Buffer;

    /**
     * Internal use only. Sends a raw packet through the radio (assumes RSSI appened to packet)
     */
    //% async shim=rf::sendRawPacket
    function sendRawPacket(msg: Buffer): void;

    /**
     * Used internally by the library.
     */
    //% help=rf/on-data-received
    //% weight=0
    //% blockId=rf_datagram_received_event block="rf on data received" blockGap=8
    //% deprecated=true blockHidden=1 shim=rf::onDataReceived
    function onDataReceived(body: () => void): void;

    /**
     * Sets the group id for radio communications. A micro:bit can only listen to one group ID at any time.
     * @param id the group id between ``0`` and ``255``, eg: 1
     */
    //% help=rf/set-group
    //% weight=100
    //% blockId=rf_set_group block="rf set group %ID"
    //% id.label="value"
    //% id.min=0 id.max=255
    //% group="Group" shim=rf::setGroup
    function setGroup(id: int32): void;

    /**
     * Change the output power level of the transmitter to the given value.
     * @param power a value in the range 0..7, where 0 is the lowest power and 7 is the highest. eg: 7
     */
    //% help=rf/set-transmit-power
    //% weight=9 blockGap=8
    //% blockId=rf_set_transmit_power block="rf set transmit power %power"
    //% power.label="value"
    //% power.min=0 power.max=7
    //% advanced=true shim=rf::setTransmitPower
    function setTransmitPower(power: int32): void;

    /**
     * Change the transmission and reception band of the radio to the given channel
     * @param band a frequency band in the range 0 - 140. Each step is 1MHz wide, based at 2360MHz.
     **/
    //% help=rf/set-frequency-band
    //% weight=8 blockGap=8
    //% blockId=rf_set_frequency_band block="rf set frequency band %band"
    //% band.label="value"
    //% band.min=0 band.max=140
    //% advanced=true shim=rf::setFrequencyBand
    function setFrequencyBand(band: int32): void;

    /**
     * Turns raw "monitor mode" on or off. While on, the radio stops enforcing
     * the micro:bit packet framing (address match, whitening, CRC) so
     * rf.readRawAntennaPacket() can see whatever is actually on the air on the
     * current channel - not just valid micro:bit packets. Normal rf.on()/send()/
     * receive still work as before once this is turned back off.
     * @param enabled true to start sniffing raw bytes, false to return to normal mode
     */
    //% help=rf/set-promiscuous-mode
    //% weight=7 blockGap=8
    //% blockId=rf_set_promiscuous_mode block="rf set promiscuous mode %enabled"
    //% advanced=true shim=rf::setPromiscuousMode
    function setPromiscuousMode(enabled: boolean): void;

    /**
     * Whether raw promiscuous/monitor mode is currently on.
     */
    //% help=rf/is-promiscuous-mode
    //% weight=6 blockGap=8
    //% blockId=rf_is_promiscuous_mode block="rf promiscuous mode on"
    //% advanced=true shim=rf::isPromiscuousMode
    function isPromiscuousMode(): boolean;

    /**
     * Measures the current energy on the antenna at the active channel, in dBm,
     * independently of whether any recognisable packet is present. This is the
     * same "spectrum scanner" style reading other 2.4GHz radios expose - it
     * does not require promiscuous mode and does not decode anything.
     * @returns signal strength in dBm (negative; closer to 0 = stronger), or 0 if unavailable
     */
    //% help=rf/scan-rssi
    //% weight=10 blockGap=8
    //% blockId=rf_scan_rssi block="rf scan rssi"
    //% advanced=true shim=rf::scanRSSI
    function scanRSSI(): int32;

    /**
     * Internal use only. While promiscuous mode is on, returns whatever raw
     * bytes were last captured off the air on the current channel, together
     * with their RSSI - regardless of whether they form a valid micro:bit
     * packet. Call rf.setPromiscuousMode(true) first.
     * @returns NULL if promiscuous mode is off or nothing has been captured yet
     */
    //% shim=rf::readRawAntennaPacket
    function readRawAntennaPacket(): Buffer;
}

// Auto-generated. Do not edit. Really.
