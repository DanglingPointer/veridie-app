package com.vasilyev.veridie;

import android.os.HandlerThread;
import android.support.test.runner.AndroidJUnit4;

import com.vasilyev.veridie.interop.Bridge;
import com.vasilyev.veridie.interop.Command;
import com.vasilyev.veridie.interop.CommandHandler;
import com.vasilyev.veridie.interop.Event;
import com.vasilyev.veridie.utils.Cast;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.concurrent.SynchronousQueue;
import java.util.concurrent.TimeUnit;

import static org.junit.Assert.*;

@RunWith(AndroidJUnit4.class)
public class InteropTest
{
   static {
      System.loadLibrary("jni_only_veridie");
   }

   public static class CommandHandlerThread extends HandlerThread
   {
      private final SynchronousQueue<Command> m_receivedCommands = new SynchronousQueue<>();
      private CommandHandler m_handler;

      public CommandHandlerThread() {
         super("UnwantedButNecessaryThread");
      }
      public CommandHandler getHandler() {
         assertNotNull(m_handler);
         return m_handler;
      }
      public Command getNextCommand() {
         try {
            return m_receivedCommands.poll(5, TimeUnit.SECONDS);
         } catch (InterruptedException e) {
            fail(e.getMessage());
            return null;
         }
      }
      @Override
      public synchronized void start() {
         super.start();
         m_handler = new CommandHandler(getLooper()) {
            @Override
            protected void onCommandReceived(Command cmd) {
               try {
                  m_receivedCommands.put(cmd);
               } catch (InterruptedException e) {
                  fail(e.getMessage());
               }
            }
         };
      }
   }

   private CommandHandlerThread cht;

   @Before
   public void setupHandler() {
      cht = new CommandHandlerThread();
      cht.start();
      Bridge.setUiCmdHandler(cht.getHandler());
      Bridge.setBtCmdHandler(cht.getHandler());
   }

   @After
   public void stopHandlerThread() {
      cht.quit();
      cht = null;
   }

   @Test
   public void testMultipleArguments() {
      String[] args = { "first arg", "<second arg />", "arg ; with ; semicolons ;" };
      Bridge.send(Event.MESSAGE_RECEIVED.withArgs(args));
      Command cmd = cht.getNextCommand();
      assertNotNull(cmd);
      assertEquals(Event.MESSAGE_RECEIVED.getId(), cmd.getId());
      assertArrayEquals(args, cmd.getArgs());
      cmd.respond(Command.ERROR_NO_ERROR);
   }

   @Test
   public void testLongArgument() {
      String veryLongArg =
         "“Two Catholics who have never met can nevertheless go together on crusade or pool funds to\n" +
         "build a hospital because they both believe that God was incarnated in human flesh and allowed\n" +
         "Himself to be crucified to redeem our sins. States are rooted in common national myths. Two\n" +
         "Serbs who have never met might risk their lives to save one another because both believe in\n" +
         "the existence of the Serbian nation, the Serbian homeland and the Serbian flag. Judicial\n" +
         "systems are rooted in common legal myths. Two lawyers who have never met can nevertheless\n" +
         "combine efforts to defend a complete stranger because they both believe in the existence of\n" +
         "laws, justice, human rights – and the money paid out in fees. Yet none of these things exists\n" +
         "outside the stories that people invent and tell one another. There are no gods in the\n" +
         "universe, no nations, no money, no human rights, no laws, and no justice outside the common\n" +
         "imagination of human beings.”\n";
      Bridge.send(Event.CAST_REQUEST_ISSUED.withArgs(veryLongArg));
      Command cmd = cht.getNextCommand();
      assertNotNull(cmd);
      assertEquals(Event.CAST_REQUEST_ISSUED.getId(), cmd.getId());
      assertEquals(1, cmd.getArgs().length);
      assertEquals(veryLongArg, cmd.getArgs()[0]);
      cmd.respond(Command.ERROR_NO_ERROR);
   }

   @Test
   public void testCommandIdIncrementation() {
      Bridge.send(Event.NEW_GAME_REQUESTED);
      Command cmd1 = cht.getNextCommand();
      assertNotNull(cmd1);
      assertEquals(Event.NEW_GAME_REQUESTED.getId(), cmd1.getId());
      assertEquals(Event.NEW_GAME_REQUESTED.getId() << 8, cmd1.getUniqueId());

      Bridge.send(Event.NEW_GAME_REQUESTED);
      Command cmd2 = cht.getNextCommand();
      assertNotNull(cmd2);
      assertEquals(Event.NEW_GAME_REQUESTED.getId(), cmd2.getId());
      assertEquals((Event.NEW_GAME_REQUESTED.getId() << 8) + 1, cmd2.getUniqueId());

      Bridge.send(Event.NEW_GAME_REQUESTED);
      Command cmd3 = cht.getNextCommand();
      assertNotNull(cmd3);
      assertEquals(Event.NEW_GAME_REQUESTED.getId(), cmd3.getId());
      assertEquals((Event.NEW_GAME_REQUESTED.getId() << 8) + 2, cmd3.getUniqueId());

      cmd1.respond(Command.ERROR_NO_ERROR);
      cmd2.respond(Command.ERROR_NO_ERROR);
      cmd3.respond(Command.ERROR_NO_ERROR);

      Bridge.send(Event.NEW_GAME_REQUESTED);
      Command cmd4 = cht.getNextCommand();
      assertNotNull(cmd4);
      assertEquals(Event.NEW_GAME_REQUESTED.getId(), cmd4.getId());
      assertEquals(Event.NEW_GAME_REQUESTED.getId() << 8, cmd4.getUniqueId());
      cmd4.respond(Command.ERROR_NO_ERROR);
   }

   @Test
   public void testLocalRefOverflow() {
      for (int i = 0; i < 512; ++i) {
         Bridge.send(Event.CAST_REQUEST_ISSUED.withArgs("D100", Integer.toString(i), "4"));
         Command cmd = cht.getNextCommand();
         assertNotNull(cmd);
         assertEquals(Event.CAST_REQUEST_ISSUED.getId(), cmd.getId());
         assertEquals("D100", cmd.getArgs()[0]);
         assertEquals(Integer.toString(i), cmd.getArgs()[1]);
         assertEquals("4", cmd.getArgs()[2]);
         cmd.respond(Command.ERROR_NO_ERROR);
      }
   }

   @Test
   public void testCastResultIsParsedCorrectly()
   {
      Cast.Result r = new Cast.Result("D6", "1;2;3;4;5;6;", 3);
      assertEquals("D6", r.getD());
      assertArrayEquals(new int[] {1, 2, 3, 4, 5, 6}, r.getValues());
      assertTrue(r.hasSuccessCount());
      assertEquals(Integer.valueOf(3), r.getSuccessCount());
   }
}
