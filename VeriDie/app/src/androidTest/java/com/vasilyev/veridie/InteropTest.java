package com.vasilyev.veridie;

import android.support.test.runner.AndroidJUnit4;

import com.vasilyev.veridie.interop.BluetoothBridge;
import com.vasilyev.veridie.interop.UiBridge;

import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.concurrent.SynchronousQueue;
import java.util.concurrent.TimeUnit;

import static android.bluetooth.BluetoothAdapter.SCAN_MODE_CONNECTABLE;
import static android.bluetooth.BluetoothAdapter.SCAN_MODE_CONNECTABLE_DISCOVERABLE;
import static android.bluetooth.BluetoothAdapter.SCAN_MODE_NONE;
import static org.junit.Assert.*;

// Tests of Java --> C++ interop. Requires showToast() working properly
@RunWith(AndroidJUnit4.class)
public class InteropTest
{
   static {
      System.loadLibrary("jni_only_veridie");
   }

   static class ToastListener implements UiBridge.Listener
   {
      SynchronousQueue<String> toasts;
      String get() {
         try {
            return toasts.poll(5, TimeUnit.SECONDS);
         } catch (InterruptedException e) {
            fail(e.getMessage());
            return null;
         }
      }
      ToastListener() {
         this.toasts = new SynchronousQueue<>();
      }
      @Override
      public void showToast(String message, int duration) {
         assertEquals(2, duration);
         toasts.offer(message);
      }
      @Override
      public void showCandidates(String[] names) {
         fail("showCandidates");
      }
      @Override
      public void showConnections(String[] names) {
         fail("showConnections");
      }
      @Override
      public void showCastResponse(int[] result, int successCount, boolean external) {
         fail("showCastResponse");
      }
      @Override
      public void showLocalName(String ownName) {
         fail("showLocalName");
      }
   }

   @Test
   public void testUiBridge() {
      ToastListener l = new ToastListener();
      UiBridge.getInstance().setListener(l);

      UiBridge bridge = UiBridge.getInstance();

      bridge.queryConnectedDevices();
      assertEquals("OnDevicesQuerytruefalse", l.get());

      bridge.queryDiscoveredDevices();
      assertEquals("OnDevicesQueryfalsetrue", l.get());

      bridge.setPlayerName("Marvin");
      assertEquals("OnNameSetMarvin", l.get());

      bridge.queryPlayerName();
      assertEquals("OnLocalNameQuery", l.get());

      bridge.requestCast(CastRequest.newD6CastRequest(42, 3));
      assertEquals("OnCastRequestD6423", l.get());

      bridge.setApprovedCandidate("Some-mac-addr");
      assertEquals("OnCandidateApprovedSome-mac-addr", l.get());

      bridge.startNewGame();
      assertEquals("OnNewGame", l.get());

      bridge.restoreState();
      assertEquals("OnRestoringState", l.get());

      bridge.saveState();
      assertEquals("OnSavingState", l.get());
   }

   @Test
   public void testBtBridge() {
      ToastListener l = new ToastListener();
      UiBridge.getInstance().setListener(l);

      BluetoothBridge bridge = BluetoothBridge.getInstance();

      bridge.onOffState(true);
      assertEquals("OnBluetoothOn", l.get());

      bridge.onOffState(false);
      assertEquals("OnBluetoothOff", l.get());

      bridge.discoverabilityResponse(true);
      assertEquals("OnDiscoverabilityConfirmed", l.get());

      bridge.discoverabilityResponse(false);
      assertEquals("OnDiscoverabilityRejected", l.get());

      bridge.discoverabilityChanged(SCAN_MODE_NONE);
      assertEquals("OnScanModeChangedfalsefalse", l.get());

      bridge.discoverabilityChanged(SCAN_MODE_CONNECTABLE_DISCOVERABLE);
      assertEquals("OnScanModeChangedtruetrue", l.get());

      bridge.discoverabilityChanged(SCAN_MODE_CONNECTABLE);
      assertEquals("OnScanModeChangedfalsetrue", l.get());
   }
}
