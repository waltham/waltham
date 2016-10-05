/*
 * Copyright © 2016 DENSO CORPORATION
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <waltham-object.h>
#include <waltham-client.h>
//#include <waltham-client-protocol.h>

#ifndef __WALTHAM_CONNECTION_H__
#define __WALTHAM_CONNECTION_H__

#define APICALL __attribute__ ((visibility ("default")))

/** A Waltham connection
 *
 * This represents the TCP connection carrying Waltham messages.
 *
 * Once the connection goes into error state due to remote timeout,
 * disconnect, Waltham protocol error, or any other reason, the various
 * functions will start returning errors.
 *
 * A Waltham client creates a wth_connection with wth_connect_to_server().
 * A Waltham server creates a wth_connection with wth_accept().
 *
 * You set up polling with the help of wth_connection_get_fd(). When
 * the fd becomes readable, you call wth_connection_read() followed by
 * wth_connection_dispatch().
 *
 * After you have sent protocol messages, wth_connection_flush() must
 * be called to flush the messages to the network before you wait for
 * incoming messages.
 */
struct wth_connection;


/** Connect to a remote Waltham server
 *
 * \param host The address of the server.
 * \param port The port of the server.
 * \return A new connection, or NULL on failure.
 *
 * This creates a client-side wth_connection, making a connection to a
 * remote Waltham server.
 *
 * On failure, errno is set.
 *
 * XXX: This call may block.
 * We should find a way to make this a non-blocking operation, so that
 * if the connection fails to form, we get an error back from read
 * and flush etc.
 */
struct wth_connection *
wth_connect_to_server(const char *host, const char *port) APICALL;

/** Accept a Waltham client connection
 *
 * \param sockfd A listening socket file descriptor to extract a
 * connection from.
 * \param addr Address of the connecting client.
 * \param addrlen Size of addr.
 *
 * See accept(2) for the documentation of the arguments.
 *
 * This creates a server-side wth_connection, representing a single
 * connected client, by accepting a new connection from the listening
 * socket.
 */
struct wth_connection *
wth_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) APICALL;

enum wth_connection_side {
	WTH_CONNECTION_SIDE_CLIENT,
	WTH_CONNECTION_SIDE_SERVER
};

/** Use an already connected socket
 *
 * \param fd A network TCP socket fd of an existing connection.
 * \param side Defines whether the local side is a server or a client.
 * \return A new wth_connection, or NULL on failure.
 *
 * This function is used to wrap an already connected file descriptor
 * in a wth_connection. The wth_connection will own a duplicate of the
 * fd, not the fd passed in. The user must close its fd after calling
 * this.
 *
 * XXX: You could probably use this as an internal helper too,
 * for the common things of wth_connect_to_server() and wth_accept().
 * But this is completely optional otherwise.
 */
struct wth_connection *
wth_connection_from_fd(int fd, enum wth_connection_side side) APICALL;

/** Get connection file descriptor
 *
 * \param conn The Waltham connection.
 * \return The file descriptor associated with the connection.
 *
 * The returned fd can be used for polling for readable and writable,
 * so that wth_connection_read() and wth_connection_flush() can be
 * called at the needed times.
 *
 * If there is no meaningful fd, e.g. due errors, -1 is returned
 * instead.
 *
 * The fd will remain owned by the wth_connection.
 */
int
wth_connection_get_fd(struct wth_connection *conn) APICALL;

/** Get the Waltham display object
 *
 * \param conn The Waltham connection.
 * \return The Waltham display object.
 *
 * The wth_display is the fundamental protocol object in Waltham. It
 * always exists if a connection exists. This function returns a
 * pointer to wth_display, but it remains owned by the wth_connection.
 *
 * The wth_display gets destroyed when the wth_connection is destroyed.
 *
 * This method is only used by clients.
 */
struct wth_display *
wth_connection_get_display(struct wth_connection *conn) APICALL;

/** Get an id to use in a message
 *
 * \param conn The Waltham connection.
 * \return The next message id.
 *
 * Return the next id, to use in a new message.
 */
int
wth_connection_get_next_message_id(struct wth_connection *conn) APICALL;

/** Disconnect
 *
 * \param conn The Waltham connection.
 *
 * Disconnects if still connected, and destroys the wth_connection.
 * The wth_display gets destroyed too.
 *
 * This call does not block.
 */
void
wth_connection_destroy(struct wth_connection *conn) APICALL;

/** Flush buffered messages to the network
 *
 * \param conn The Waltham connection.
 * \return The number of bytes written to network, or -1 on failure.
 *
 * Attempts to send all buffered messages to the network. It writes as
 * much data as possible without blocking. If any data remains unsent,
 * -1 is returned and errno is set to EAGAIN. In case of EAGAIN, poll
 * for writable and flush again.
 *
 * This call does not block. On error, errno is set.
 */
int
wth_connection_flush(struct wth_connection *conn) APICALL;

/** Read data received from the network
 *
 * \param conn The Waltham connection.
 * \return 0 on success, -1 on failure with errno set.
 *
 * Reads as much data as available on the network socket into internal
 * buffers. To actually dispatch incoming messages, use
 * wth_connection_dispatch() after reading.
 *
 * This call does not block. If no data is available, the call
 * returns immediately with success.
 */
int
wth_connection_read(struct wth_connection *conn) APICALL;

/** Dispatch incoming messages
 *
 * \param conn The Waltham connection.
 * \return The number of dispatched messages, or -1 on error.
 *
 * This function calls the message handlers (listeners, interface
 * callbacks) for buffered messages. The messages must have been
 * buffered by an earlier call to wth_connection_read() as no reading
 * from the network is attempted.
 *
 * On error, errno is set. For nothing to dispatch, 0 is returned.
 */
int
wth_connection_dispatch(struct wth_connection *conn) APICALL;

/** Make a roundtrip from client
 *
 * \param conn The Waltham connection.
 *
 * This is a helper for creating a wthp_callback object by sending
 * wth_display.sync request, and waiting for the callback to trigger.
 * As such, it is only usable for clients.
 *
 * This call blocks until the roundtrip completes.
 *
 * XXX: This does not actually need to be implemented in Waltham, it
 * could be implemented in the user as well.
 */
int
wth_roundtrip(struct wth_connection *conn) APICALL;

#endif /* __WALTHAM_CONNECTION_H__ */
