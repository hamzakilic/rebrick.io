#include "./rebrick_async_udpsocket.h"
#include "./server_client/udpecho.h"
#include "cmocka.h"
#include "unistd.h"

#define UDPSERVER_PORT "9999"
static int setup(void **state)
{
    unused(state);
    fprintf(stdout, "****  %s ****\n", __FILE__);
    int32_t result = udp_echo_start(atoi(UDPSERVER_PORT));

    return result;
}

static int teardown(void **state)
{
    unused(state);
    udp_echo_close();
    uv_loop_close(uv_default_loop());
    return 0;
}

static int32_t flag = 0;
static char read_buffer[65536] = {'\0'};
static const char *testdata = "merhaba";

static int32_t on_error_occured(rebrick_async_socket_t *socket, void *data, int error)
{
    unused(socket);
    unused(data);
    unused(error);
    return REBRICK_SUCCESS;
}
static int32_t on_server_received(rebrick_async_socket_t *socket, void *data, const struct sockaddr *addr, const char *buffer, ssize_t len)
{
    unused(addr);
    unused(socket);

    assert_string_equal(data, testdata);
    flag = 1;
    memset(read_buffer, 0, 512);
    memcpy(read_buffer, buffer, len < 65536 ? len : 65536);

    return REBRICK_SUCCESS;
}

static int32_t on_server_send(rebrick_async_socket_t *socket, void *data, void *source, int status)
{
    unused(data);
    unused(socket);
    unused(source);
    assert_string_equal(data, testdata);
    flag = 2;

    return REBRICK_SUCCESS + status - status;
}

static void rebrick_async_udpsocket_asserver_communication(void **start)
{
    unused(start);
    rebrick_async_udpsocket_t *server;
    const char *bind_ip = "0.0.0.0";
    const char *bind_port = "9090";
    rebrick_sockaddr_t bind;
    rebrick_util_to_socket(&bind, bind_ip, bind_port);

    const char *localhost_ip = "127.0.0.1";

    rebrick_sockaddr_t localhost;
    rebrick_util_to_socket(&localhost, localhost_ip, bind_port);

    const char *dest_ip = "127.0.0.1";
    const char *dest_port = "9999";
    rebrick_sockaddr_t client;
    rebrick_util_to_socket(&client, dest_ip, dest_port);

    int32_t result = rebrick_async_udpsocket_new(&server, bind, cast(testdata, void *), on_server_received, on_server_send, on_error_occured);
    assert_int_equal(result, 0);
    //check loop
    uv_run(uv_default_loop(), UV_RUN_NOWAIT);

    const char *msg = "hello world";

    flag = 0;
    udp_echo_send2(msg, &localhost.v4);
    //loop again
    uv_run(uv_default_loop(), UV_RUN_NOWAIT);
    //check for received data
    int32_t max_check = 10;
    while (!flag && max_check)
    {
        usleep(10000);
        uv_run(uv_default_loop(), UV_RUN_NOWAIT);
        max_check--;
    }
    //free(buffer);
    assert_int_not_equal(max_check, 0);
    assert_string_equal(msg, read_buffer);
    flag = 0;
    char *reply = "got it";
    rebrick_clean_func_t clean = {};
    result = rebrick_async_udpsocket_send(server, &client, reply, strlen(reply) + 1, clean);
    assert_int_equal(result, 0);
    // uv_run(uv_default_loop(),UV_RUN_NOWAIT);

    //check for received data
    max_check = 10;
    while (!flag && max_check)
    {
        usleep(10000);
        uv_run(uv_default_loop(), UV_RUN_NOWAIT);
        max_check--;

        result = udp_echo_recv(read_buffer);
        if (result > 0)
            break;
    }
    assert_int_not_equal(max_check, 0);
    assert_string_equal(reply, read_buffer);

    result = rebrick_async_udpsocket_destroy(server);
    assert_int_equal(result, 0);
    max_check = 100;
    while (max_check--)
    {
        usleep(1000);
        uv_run(uv_default_loop(), UV_RUN_NOWAIT);
    }
}

static int32_t on_dnsclient_error_occured(rebrick_async_socket_t *socket, void *data, int32_t error)
{

    unused(socket);
    unused(data);
    unused(error);
    return REBRICK_SUCCESS;
}

static int32_t received_count = 0;
static int32_t on_dnsclient_received(rebrick_async_socket_t *socket, void *data, const struct sockaddr *addr, const char *buffer, ssize_t len)
{
    unused(addr);
    unused(socket);
    unused(data);
    unused(buffer);
    unused(len);

    received_count++;
    return REBRICK_SUCCESS;
}
static int32_t sended_count = 0;
static int32_t on_dnsclient_send(rebrick_async_socket_t *socket, void *data, void *source, int status)
{
    unused(data);
    unused(socket);
    unused(source);
    unused(data);
    unused(status);
    sended_count++;
    return REBRICK_SUCCESS + status - status;
}

/////////////////////// memory tests ///////////////////////////////////////////////

/**
 * @brief create socket, send a packet, then destory socket
 * and test this situation more
 *
 * @param state
 */
static void test_rebrick_async_udpsocket_check_memory(void **state)
{
    //try sending the same dnsd packet
    //to dockerized bind server
    //and check memory of program

    //create and send sockets much

    unused(state);
    //read a sample dns packet
    char *testdata;
    size_t datalen;

    int32_t result = rebrick_util_file_read_allbytes("./test/testdata/testpacket1.bin", &testdata, &datalen);
    if (result)
        result = rebrick_util_file_read_allbytes("./testdata/testpacket1.bin", &testdata, &datalen);
    assert_int_equal(datalen, 48);

    const char *dest_ip = "127.0.0.1";
    const char *dest_port = "5555";
    rebrick_sockaddr_t destination;
    rebrick_util_to_socket(&destination, dest_ip, dest_port);

    rebrick_sockaddr_t bindaddr;
    rebrick_util_to_socket(&bindaddr, "0.0.0.0", "0");
    rebrick_async_udpsocket_t *dnsclient;

#define COUNTER 250
    for (int i = 0; i < COUNTER; ++i)
    {

        result = rebrick_async_udpsocket_new(&dnsclient, bindaddr, NULL, on_dnsclient_received, on_dnsclient_send, on_dnsclient_error_occured);
        assert_int_equal(result, 0);

        sended_count = 0;
        received_count = 0;
        rebrick_clean_func_t clean = {};
        rebrick_async_udpsocket_send(dnsclient, &destination, testdata, datalen, clean);
        int counter = 1000;
        while (counter-- && !sended_count)
        {
            usleep(100);
            uv_run(uv_default_loop(), UV_RUN_NOWAIT);
        }
        assert_int_equal(sended_count, 1);
        //data sended

        counter = 1000;
        while (counter-- && !received_count)
        {

            uv_run(uv_default_loop(), UV_RUN_NOWAIT);
            usleep(1000);
        }
        assert_int_equal(received_count, 1);
        rebrick_async_udpsocket_destroy(dnsclient);
        counter = 100;
        while (counter)
        {
            usleep(100);
            uv_run(uv_default_loop(), UV_RUN_NOWAIT);
            counter--;
        }
    }
    free(testdata);
}

/**
 * @brief create a socket
 * send lots of packets
 * at the end destory socket
 *
 * @param state
 */
static void test_rebrick_async_udpsocket_check_memory2(void **state)
{
    //try sending the same dnsd packet
    //to dockerized bind server
    //and check memory of program

    //create socket once, send packets mode

    unused(state);

    //read a sample dns packet
    char *testdata;
    size_t datalen;

    int32_t result = rebrick_util_file_read_allbytes("./test/testdata/testpacket1.bin", &testdata, &datalen);
    if (result)
        result = rebrick_util_file_read_allbytes("./testdata/testpacket1.bin", &testdata, &datalen);
    assert_int_equal(datalen, 48);

    const char *dest_ip = "127.0.0.1";
    const char *dest_port = "5555";
    rebrick_sockaddr_t destination;
    rebrick_util_to_socket(&destination, dest_ip, dest_port);

    rebrick_sockaddr_t bindaddr;
    rebrick_util_to_socket(&bindaddr, "0.0.0.0", "0");
    rebrick_async_udpsocket_t *dnsclient;

    result = rebrick_async_udpsocket_new(&dnsclient, bindaddr, NULL, on_dnsclient_received, on_dnsclient_send, on_dnsclient_error_occured);
    assert_int_equal(result, 0);

#define COUNTER 250
    for (int i = 0; i < COUNTER; ++i)
    {

        sended_count = 0;
        received_count = 0;
        rebrick_clean_func_t clean = {};
        rebrick_async_udpsocket_send(dnsclient, &destination, testdata, datalen, clean);

        int counter = 10000;
        while (counter-- && !sended_count)
        {
            usleep(100);
            uv_run(uv_default_loop(), UV_RUN_NOWAIT);
        }
        assert_int_equal(sended_count, 1);
        //data sended

        counter = 1000;
        while (counter-- && !received_count)
        {
            usleep(1000);
            uv_run(uv_default_loop(), UV_RUN_NOWAIT);
        }
        assert_true(received_count > 0);
    }
    rebrick_async_udpsocket_destroy(dnsclient);
    int32_t counter = 100;
    while (counter)
    {
        usleep(1000);
        uv_run(uv_default_loop(), UV_RUN_NOWAIT);
        counter--;
    }
    free(testdata);
}

/**
 * @brief create a udp server and send packets with hping3
 * hping3 --flood --rand-source --udp -d 25  -p TARGET_PORT TARGET_IP
 * @param state
 */
static void test_rebrick_async_udpsocket_check_memory3(void **state)
{
    //create a udp server
    //and send packets with hping3

    unused(state);
    rebrick_sockaddr_t bindaddr;
    rebrick_util_to_socket(&bindaddr, "0.0.0.0", "9595");
    rebrick_async_udpsocket_t *dnsclient;

    int32_t result = rebrick_async_udpsocket_new(&dnsclient, bindaddr, NULL, on_dnsclient_received, NULL, on_dnsclient_error_occured);
    assert_int_equal(result, 0);

    received_count = 0;
    //istenirse burası ile memory test yapılabilir
#define COUNTER2 250
    for (int i = 0; i < COUNTER2; ++i)
    {

        sended_count = 0;
        uv_run(uv_default_loop(), UV_RUN_NOWAIT);
    }

    rebrick_async_udpsocket_destroy(dnsclient);
    int32_t counter = 100;
    while (counter)
    {
        usleep(1000);
        uv_run(uv_default_loop(), UV_RUN_NOWAIT);
        counter--;
    }
    //assert_true(received_count > 0);
}

int test_rebrick_async_udpsocket(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(rebrick_async_udpsocket_asserver_communication),
        cmocka_unit_test(test_rebrick_async_udpsocket_check_memory),
        cmocka_unit_test(test_rebrick_async_udpsocket_check_memory2),
        cmocka_unit_test(test_rebrick_async_udpsocket_check_memory3)};
    return cmocka_run_group_tests(tests, setup, teardown);
}
