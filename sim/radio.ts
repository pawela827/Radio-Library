namespace pxsim.rf {
    export function raiseEvent(id: number, eventid: number): void {
        const state = pxsim.getRFState();
        state.raiseEvent(id, eventid);
    }

    export function setGroup(id: number): void {
        const state = pxsim.getRFState();
        state.setGroup(id);
    }

    export function setTransmitPower(power: number): void {
        const state = pxsim.getRFState();
        state.setTransmitPower(power);
    }

    export function setFrequencyBand(band: number) { 
        const state = pxsim.getRFState();
        state.setFrequencyBand(band);
    }

    export function sendRawPacket(buf: RefBuffer) {
        let cb = getResume();
        const state = pxsim.getRFState();
        if (state.enable) {
            state.datagram.send({
                type: 0,
                groupId: state.groupId,
                bufferData: buf.data
            });
        }
        setTimeout(cb, 1);
    }

    export function readRawPacket() {
        const state = pxsim.getRFState();
        const packet = state.datagram.recv();
        const buf = packet.payload.bufferData;
        const n = buf.length;
        if (!n)
            return undefined;

        const rbuf = BufferMethods.createBuffer(n + 4);
        for(let i = 0; i < buf.length; ++i)
            rbuf.data[i] = buf[i];
        // append RSSI
        BufferMethods.setNumber(rbuf, BufferMethods.NumberFormat.Int32LE, n, packet.rssi)
        return rbuf;
    }

    export function onDataReceived(handler: RefAction): void {
        const state = pxsim.getRFState();
        state.datagram.onReceived(handler);
    }

    export function setProtocol(protocol: number): void {
        const state = pxsim.getRFState();
        state.setProtocol(protocol);
    }

    export function getProtocol(): number {
        const state = pxsim.getRFState();
        return state.protocol;
    }

    export function scanRSSI(): number {
        const state = pxsim.getRFState();
        return state.scanRSSI();
    }

    export function readRawAntennaPacket() {
        const state = pxsim.getRFState();
        const packet = state.promiscuousDatagram.recv();
        const buf = packet.payload.bufferData;
        const n = buf.length;
        if (!n)
            return undefined;

        const rbuf = BufferMethods.createBuffer(n + 4);
        for (let i = 0; i < buf.length; ++i)
            rbuf.data[i] = buf[i];
        // append RSSI
        BufferMethods.setNumber(rbuf, BufferMethods.NumberFormat.Int32LE, n, packet.rssi)
        return rbuf;
    }

    export function setEsbAddress(buf: RefBuffer): void {
        const state = pxsim.getRFState();
        if (buf && buf.data && buf.data.length === 5)
            state.setEsbAddress(buf.data);
    }

    export function sendRawAntennaPacket(protocol: number, buf: RefBuffer) {
        const state = pxsim.getRFState();
        if (!state.enable || !buf) return;
        if (protocol === pxsim.RF_PROTOCOL_MAKECODE || protocol === pxsim.RF_PROTOCOL_GAZELL) return;

        if (protocol !== state.protocol)
            state.setProtocol(protocol);
        if (protocol !== state.protocol) return; // rejected (unknown/unavailable)

        // broadcast like a real raw-capable protocol would: no groupId
        // filtering, picked up by anything else in promiscuous/Esb mode -
        // see RFState.receivePacket()'s promiscuousDatagram branch
        state.promiscuousDatagram.send({
            type: -1,
            groupId: state.groupId,
            bufferData: buf.data
        });
    }

    export function getFrequencyMHz(): number {
        const state = pxsim.getRFState();
        // simulator has no real hardware register - report what setFrequencyBand() stored
        return 2360 + state.band;
    }

    export function off(){
        const state = pxsim.getRFState();
        state.off();
    }

    export function on(){
        const state = pxsim.getRFState();
        state.on();
    }

}
(pxsim as any).rf = pxsim.rf;
