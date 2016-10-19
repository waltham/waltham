Waltham IPC Library                                      {#mainpage}
===================

[Waltham] is a network IPC library designed to resemble [Wayland] both
protocol and protocol-API wise. Protocol is described in XML files. A
generator translates XML into C code at build time. One designs Waltham
protocols exactly the same way as Wayland extensions, you just miss the
file descriptor argument type.

The protocol framework is designed to be completely asynchronous and
object-oriented, just like Wayland. To make synchronous calls one has
to write their protocol definition specifically to do that, there is no
shortcut offered.

The goal is to have all protocol definitions to be external, so that a
`libwaltham` compiled once can be used by any project with any protocol
definition. Waltham itself would carry just the control protocol
definition and public interfaces for `wthp_callback` and
`wthp_registry`, which each user project would implement on their own.
User projects will then run the generator and build in the
protocol-specific bits of code and API. *The implementation is not this
far yet, see [TODO](TODO).*

Waltham does not (yet) support any kind of encryption, so usage in
untrusted networks is not advised. Help with securing the
communications would be appreciated, starting with planning on how it
should be done. In the mean time, secure tunnels are recommended (e.g.
`ssh`).

**Waltham ABI and API are not yet stable.** Breakage will likely occur
until Waltham 1.0.0 has been released. You are still welcome to install
Waltham and try it in your projects.


Documentation
=============

In addition to this README, the project can build API documentation
with Doxygen. Build the project as described below, and you should find
the documentation in your `@top_builddir@/doc/html/index.html`. This
README is mirrored on the front page. If the documentation does not
seem to get built, try passing `--enable-doc` to `./autogen.sh` or
`./configure` and see what it complains about.


Building and testing
====================

To build:
```
$ ./autogen.sh
$ make
```

To test, first start the server:
```
$ ./tests/server
```

Then in another terminal, run the client:
```
$ ./tests/client
```

The client runs for about five seconds excercising the protocol and
exits automatically. The server can be stopped with `ctrl+c`.

If you want to run the example server or client under GDB or Valgrind,
use e.g. the following:
```
$ libtool --mode=execute gdb ./tests/server
$ libtool --mode=execute valgrind -v --leak-check=full --show-reachable=yes --track-origins=yes --num-callers=30 ./tests/server
```


Design
======

API
---

The Waltham API revolves around two fundamental types: `wth_connection`
and `wth_object`. Both have the same implementation on both server
and client side, although the possible operations (functions to call)
vary. Once created, both types are tied to being either server or
client side instances.

`wth_connection` represents the TCP connection. For a client it
represents the connection to a server, for a server it represents one
client.

`wth_object` represents a protocol object. However, usually user code
does not handle wth_objects directly, but works with the API generated
from XML files which uses opaque pointer types. These opaque pointer
types are simply cast to/from `wth_object` under the hood. For some
uncommon operations that the generator does not create wrappers for,
you need to cast to `(struct wth_object *)` yourself.

Threading considerations
------------------------

Waltham does not carry features to help making threaded programs
easier. However, it should be safe if you restrict all manipulations of
a `wth_connection` and all `wth_objects` associated with it to a single
thread at a time.

Message handling and object lifetimes
-------------------------------------

Incoming messages, both on the server and client side, are first stored
in a raw data buffer. Messages are dispatched directly from the data
buffer. This is different from Wayland, which demarshals messages from
its data buffer and puts them into queues before dispatching per queue.
The additional queueing in Wayland adds some complexity that Waltham
avoids.

Waltham does not include any automatic object destruction when a
`wth_connection` is destroyed. User code must track and destroy all
associated `wth_object`s before destroying the `wth_connection`. This
may seem awkward at first, but it gives the user code the opportunity
to destroy `wth_object`s in any specific order it prefers and without
the risk of corrupting lists. Compare this to Wayland, where one has to
be able to deal with any arbitrary order of destruction between all
`wl_resource`s, and the destructor path of one `wl_resource` cannot
remove any other destroy listeners that might be on the same destroy
listener list as that would lead to list corruption.

`wth_object` instances are created automatically during dispatch, which
means that messages not yet dispatched have not created any entries in
the table that maps between object IDs on the wire and `wth_object`
instances. Therefore objects do not need to be cleaned up until the
messages creating them have been dispatched, so there is no need for
destructor hooks in `wth_object`.

Main loop integration
---------------------

Waltham does not have any kind of event loop implementation itself. For
every `wth_connection`, you get a file descriptor you need to watch and
then call the appropriate Waltham functions which are guaranteed to
never block (unless the user provides message handlers that block).

This design was chosen to ease porting of Waltham to other operating
systems and allow users to use any main loop implementations they wish.
Even though the examples use Linuxisms like `epoll` and `timerfd`, the
library does not. This also avoids imposing a dependency to any
particular external event loop library. It makes `libwaltham` a much
thinner library than `libwayland-server`.


[Waltham]: https://a.github.url.invalid
[Wayland]: https://wayland.freedesktop.org/
