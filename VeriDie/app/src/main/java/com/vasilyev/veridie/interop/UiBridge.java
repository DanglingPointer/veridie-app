package com.vasilyev.veridie.interop;

import android.util.Log;

import com.vasilyev.veridie.CastRequest;

public class UiBridge
{
    private static final int NO_ERROR = 0;
    private static final int ERROR_NO_LISTENER = 1;
    private static final int ERROR_UNHANDLED_EXCEPTION = 2;

    private static final String TAG = UiBridge.class.getName();

    public interface Listener
    {
        void showToast(String message, int duration);
        void showCandidates(String[] names);
        void showConnections(String[] names);
        void showCastResponse(int[] result, int successCount, boolean external);
        void showLocalName(String ownName);
    }

    private static UiBridge s_instance = null;
    public static UiBridge getInstance() {
        if (s_instance == null)
            s_instance = new UiBridge();
        return s_instance;
    }

    private Listener m_listener;
    private UiBridge() {}
    public void setListener(Listener l) {
        m_listener = l;
        bridgeCreated();
    }

    /** C++ --> JAVA */

    private static int showToast(String message, int duration) {
        try {
            s_instance.m_listener.showToast(message, duration);
            return NO_ERROR;
        } catch (NullPointerException e) {
            return ERROR_NO_LISTENER;
        } catch (Exception e) {
            Log.e(TAG, e.toString());
            return ERROR_UNHANDLED_EXCEPTION;
        }
    }
    private static int showCandidates(String[] names) {
        try {
            s_instance.m_listener.showCandidates(names);
            return NO_ERROR;
        } catch (NullPointerException e) {
            return ERROR_NO_LISTENER;
        } catch (Exception e) {
            Log.e(TAG, e.toString());
            return ERROR_UNHANDLED_EXCEPTION;
        }
    }
    private static int showConnections(String[] names) {
        try {
            s_instance.m_listener.showConnections(names);
            return NO_ERROR;
        } catch (NullPointerException e) {
            return ERROR_NO_LISTENER;
        } catch (Exception e) {
            Log.e(TAG, e.toString());
            return ERROR_UNHANDLED_EXCEPTION;
        }
    }
    private static int showCastResponse(int[] result, int successCount, boolean external) {
        try {
            s_instance.m_listener.showCastResponse(result, successCount, external);
            return NO_ERROR;
        } catch (NullPointerException e) {
            return ERROR_NO_LISTENER;
        } catch (Exception e) {
            Log.e(TAG, e.toString());
            return ERROR_UNHANDLED_EXCEPTION;
        }
    }
    private static int showLocalName(String ownName) {
        try {
            s_instance.m_listener.showLocalName(ownName);
            return NO_ERROR;
        } catch (NullPointerException e) {
            return ERROR_NO_LISTENER;
        } catch (Exception e) {
            Log.e(TAG, e.toString());
            return ERROR_UNHANDLED_EXCEPTION;
        }
    }

    /** Java --> C++*/

    public void queryConnectedDevices() {
        queryDevices(true, false);
    }
    public void queryDiscoveredDevices() {
        queryDevices(false, true);
    }
    public void setPlayerName(String name) {
        setName(name);
    }
    public void queryPlayerName() {
        queryLocalName();
    }
    public void requestCast(CastRequest request) {
        castRequest(request.getD(), request.getCount(),
                request.getThreshold() == null ? -1 : request.getThreshold());
    }
    public void setApprovedCandidate(String approvedMacAddrs) {
        candidateApproved(approvedMacAddrs);
    }
    public void startNewGame() {
        newGame();
    }
    public void restoreState() {
        restoringState();
    }
    public void saveState() {
        savingState();
    }

    private static native void bridgeCreated();
    private static native void queryDevices(boolean connected, boolean discovered);
    private static native void setName(String name);
    private static native void queryLocalName();
    private static native void castRequest(int d, int count, int threshold);
    private static native void candidateApproved(String approvedMac);
    private static native void newGame();
    private static native void restoringState();
    private static native void savingState();
}
