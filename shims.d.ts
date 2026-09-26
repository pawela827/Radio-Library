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
    //% advanced=true shim=rf::setProtocol
    function setProtocol(protocol: int32): void;

    /**
     * Which protocol the radio is currently using.
     */
    //% help=rf/get-protocol
    //% weight=6 blockGap=8
    //% blockId=rf_get_protocol block="rf protocol"
    //% advanced=true shim=rf::getProtocol
    function getProtocol(): int32;

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
    //% advanced=true shim=rf::scanRSSI
    function scanRSSI(): int32;

    /**
     * Internal use only. While the radio is on a raw-capable protocol (eg.
     * RFProtocol.Raw or RFProtocol.Esb), returns whatever bytes were
     * last captured off the air on the current channel using that protocol's
     * framing, together with their RSSI.
     * @returns NULL if not on a raw-capable protocol or nothing captured yet
     */
    //% shim=rf::readRawAntennaPacket
    function readRawAntennaPacket(): Buffer;

    /**
     * Sets the 5-byte on-air address used by RFProtocol.Esb - the same role
     * as the address configured on an nRF24L01(+) module. Only takes effect
     * while ESB is the active protocol.
     * @param address exactly 5 bytes
     */
    //% help=rf/set-esb-address
    //% weight=5 blockGap=8
    //% blockId=rf_set_esb_address block="rf set esb address %address"
    //% advanced=true shim=rf::setEsbAddress
    function setEsbAddress(address: Buffer): void;

    /**
     * Sends raw bytes on-air using the given protocol's framing. Switches to
     * that protocol first if the radio isn't already on it. RFProtocol.MakeCode
     * isn't accepted here - use radio.sendNumber()/sendString()/etc instead.
     * @param protocol which protocol to send with (see RFProtocol)
     * @param data the bytes to transmit, up to 32 bytes
     */
    //% help=rf/send-raw-antenna-packet
    //% weight=4 blockGap=8
    //% blockId=rf_send_raw_antenna_packet block="rf send raw %protocol packet %data"
    //% advanced=true shim=rf::sendRawAntennaPacket
    function sendRawAntennaPacket(protocol: int32, data: Buffer): void;

    /**
     * Test/diagnostic: the frequency the RF hardware is actually set to, in MHz,
     * read straight from the chip's FREQUENCY register (including the MAP bit on
     * V2) - not from any variable in this extension. Useful to confirm that
     * rf.setFrequencyBand() really reached the hardware.
     * @returns frequency in MHz (eg. 2412), or 0 if unavailable
     */
    //% help=rf/get-frequency-mhz
    //% weight=3 blockGap=8
    //% blockId=rf_get_frequency_mhz block="rf actual frequency (MHz)"
    //% advanced=true shim=rf::getFrequencyMHz
    function getFrequencyMHz(): int32;
}

// Auto-generated. Do not edit. Really.
