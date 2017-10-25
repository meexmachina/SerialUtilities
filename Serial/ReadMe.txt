CSerial(Ex/Wnd) - Win32 message-based wrapper for serial communications

A lot of MS-Windows GUI based programs use a central message
loop, so the application cannot block to wait for objects. This
make serial communication difficult, because it isn't event
driven using a message queue. This class makes the CSerial based
classes suitable for use with such a messagequeue. Whenever
an event occurs on the serial port, a user-defined message will
be sent to a user-defined window. It can then use the standard
message dispatching to handle the event.

Pros:
-----
  - Easy to use
  - Fully ANSI and Unicode aware
  - Integrates easily in GUI applications and is intuitive to
      use for GUI application programmers

Cons:
-----
  - Uses a thread for each COM-port, which has been opened.
  - More overhead, due to thread switching and message queues.
  - Requires a window, but that's probably why you're using
    this class.

Copyright (C) 1999-2003 Ramon de Klein (Ramon.de.Klein@ict.nl)
