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

#ifndef WALTHAM_CONNECTION_H
#define WALTHAM_CONNECTION_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <waltham-object.h>

#ifdef  __cplusplus
extern "C" {
#endif

/** A Waltham connection
 *
 * This represents the TCP connection carrying Waltham messages.
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
 *
 * A server should not destroy a wth_connection before the client has
 * closed it, so that the TCP TIME_WAIT state is left on the client
 * system. In case of a protocol error, the server should call
 * wth_object_post_error() and wait for the client to disconnect.
 *
 * A client should do one final roundtrip before destroying a
 * wth_connection to ensure that all sent events have reached the
 * server.
 *
 * Once the connection goes into error state due to remote timeout,
 * disconnect, Waltham protocol error, or any other reason, the various
 * functions will start returning appropriate errors as described in
 * each of them.
 *
 * When the connection is specifically in protocol error state:
 * - reading will discard all incoming data
 * - dispatching will discard all pending messages and not dispatch
 * - sending messages still works
 * - flush still works
 * The protocol error state is only returned from
 * wth_connection_dispatch(), other functions will silently succeed
 * unless there are further errors.
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
wth_connect_to_server(const char *host, const char *port);

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
wth_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);

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
wth_connection_from_fd(int fd, enum wth_connection_side side);

/** Get connection file descriptor
 *
 * \param conn The Waltham connection.
 * \return The file descriptor associated with the connection.
 *
 * The returned fd can be used for polling for readable and writable,
 * so that wth_connection_read() and wth_connection_flush() can be
 * called at the needed times.
 *
 * The fd will remain owned by the wth_connection.
 */
int
wth_connection_get_fd(struct wth_connection *conn);

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
wth_connection_get_display(struct wth_connection *conn);

/** Get an id to use in a message
 *
 * \param conn The Waltham connection.
 * \return The next message id.
 *
 * Return the next id, to use in a new message.
 */
int
wth_connection_get_next_message_id(struct wth_connection *conn);

/** Get an id to use in an object
 *
 * \param conn The Waltham connection.
 * \return The next object id.
 *
 * Return a free object id, to use in a new object.
 */
int
wth_connection_get_next_object_id(struct wth_connection *conn);

void
wth_connection_insert_object(struct wth_connection *conn,
    struct wth_object *obj);

void
wth_connection_remove_object(struct wth_connection *conn,
    struct wth_object *obj);

struct wth_object *
wth_connection_get_object(struct wth_connection *conn, uint32_t id);

/** Disconnect
 *
 * \param conn The Waltham connection.
 *
 * Disconnects if still connected, and destroys the wth_connection.
 * The wth_display gets destroyed too.
 *
 * This call does not block. Messages still en route may get discarded.
 */
void
wth_connection_destroy(struct wth_connection *conn);

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
 * This call does not block. On failure, errno is set.
 *
 * This function does not indicate if the connection has been set as
 * failed due to a protocol error. This behaviour allows the server to
 * continue flushing events out so that the protocol error event will
 * reach the client.
 */
int
wth_connection_flush(struct wth_connection *conn);

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
 *
 * The connection being in protocol error state does not cause this
 * function to return error, but it does cause all read data to be
 * discarded.
 */
int
wth_connection_read(struct wth_connection *conn);

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
 *
 * In case the connection is set to protocol error state during this
 * call or earlier, this function will return -1 and set errno to
 * EPROTO. No further messages will be dispatched once the protocol
 * error state has been set.
 *
 * Servers should keep the connection open regardless of EPROTO errors
 * to ensure the error event gets delivered to the client.
 */
int
wth_connection_dispatch(struct wth_connection *conn);

/** Make a roundtrip from a client
 *
 * \param conn The Waltham connection.
 * \return 0 on success, -1 on failure.
 *
 * This is a helper for creating a wthp_callback object by sending
 * wth_display.sync request, and waiting for the callback to trigger.
 * As such, it is only usable for clients.
 *
 * This call blocks indefinitely until the roundtrip completes or the
 * connection fails. It will flush outgoing and dispatch incoming
 * messages until it returns.
 *
 * On failure, use wth_connection_get_error() to inspect the error.
 *
 * If blocking indefinitely is not desireable or you want to be able
 * to process other file descriptors in the mean time, you have to
 * write your own roundtrip implementation using the public API.
 */
int
wth_connection_roundtrip(struct wth_connection *conn);

/* Set wth_connection to errored state
 *
 * Once set to errored state, the connection is effectively dead.
 * All incoming data will be discarded, no messages will be dispatched,
 * and sent messages are silently dropped instead of sent. Various
 * wth_connection functions will return errors.
 *
 * This should probably not be public exported API.
 */
void
wth_connection_set_error(struct wth_connection *conn, int err);

/* Set wth_connection into protocol error state
 *
 * This implies wth_connection_set_error() with the error set to EPROTO.
 * \param object_id ID of the object referenced in the error event
 * \param interface interface name of the object referenced in the error event
 * \param error_code interface-specific error code
 */
void
wth_connection_set_protocol_error(struct wth_connection *conn,
				  uint32_t object_id,
				  const char *interface,
				  uint32_t error_code);

/** Return the connection error state
 *
 * \param conn The Waltham connection.
 * \return The error code from the 'errno' set, or zero for no error.
 *
 * If the returned code is EPROTO, wth_connection_get_protocol_error()
 * can be used to get the protocol error details.
 */
int
wth_connection_get_error(struct wth_connection *conn);

/** Return protocol error details
 *
 * \param conn The Waltham connection.
 * \param interface Return pointer to the interface name.
 * \param object_id Return the object ID.
 * \return The interface specific error code.
 *
 * The returned interface name and object ID are for the object that
 * was referenced when the server sent the error event. The returned
 * error code is specific to that interface, and zero does not mean
 * "no error".
 *
 * \sa wth_connection_get_error()
 */
uint32_t
wth_connection_get_protocol_error(struct wth_connection *conn,
				  const char **interface,
				  uint32_t *object_id);

#ifdef  __cplusplus
}
#endif

#endif
