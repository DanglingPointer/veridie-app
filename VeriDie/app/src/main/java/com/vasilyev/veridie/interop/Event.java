package com.vasilyev.veridie.interop;

public enum Event
{
    // event IDs must be in sync with core/events.hpp
    REMOTE_DEVICE_CONNECTED(10),
    REMOTE_DEVICE_DISCONNECTED(11),
    CONNECTIVITY_ESTABLISHED(12),
    NEW_GAME_REQUESTED(13),
    MESSAGE_RECEIVED(14),
    CAST_REQUEST_ISSUED(15),
    GAME_STOPPED(16),
    BLUETOOTH_ON(17),
    BLUETOOTH_OFF(18),
    SOCKET_READ_FAILED(19);

    private final int m_id;
    private String[] m_args;

    private Event(int id) {
        m_id = id;
    }
    public Event withArgs(String... args) {
        m_args = args;
        return this;
    }

    public int getId() {
        return m_id;
    }
    public String[] getArgs() {
        return m_args;
    }
}
