/* Encode/decode benchmarks for the risk (RBP) binary library, mirroring
 * bench/bench.zig in fin-proto-risk-bin-zig and examples/bench.rs in
 * fin-proto-risk-bin-rs: same methodology (10k warmup iterations, then 10
 * batches of 100k operations, fastest batch reported), so the numbers are
 * directly comparable.
 *
 * Output is one TSV line per case on stdout:
 *   lib \t lang \t op \t ns_per_op \t frame_bytes
 */
#include "root.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#define LIB "risk"
#define ITERS 100000
#define BATCHES 10
#define WARMUP 10000

static volatile uint64_t sink;

static double now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1e9 + (double)ts.tv_nsec;
}

static void report(const char *op, double best_ns, size_t bytes) {
    printf("%s\tc\t%s\t%.1f\t%zu\n", LIB, op, best_ns / (double)ITERS, bytes);
}

static void fill_new_order_req(rbp_binary_new_order_req *msg) {
    memset(msg, 0, sizeof(*msg));
    msg->req_id = 1001;
    msg->channel_id = 1;
    msg->adapter = 1;
    msg->account = FC_STR("A123");
    msg->account_group = FC_STR("G1");
    msg->cl_ord_id = FC_STR("C0001");
    msg->security_id = FC_STR("600000");
    msg->market = 1;
    msg->side = 1;
    msg->price = 1050;
    msg->order_qty = 100;
    msg->ord_type = 1;
    msg->time_in_force = 0;
    msg->pbu = FC_STR("P0001");
    msg->transact_time = 1700000000;
    msg->ts_recv_ns = 123456;
    msg->raw_ref = 7;
}

static size_t bench_encode(void) {
    rbp_binary_new_order_req msg;
    fill_new_order_req(&msg);
    fc_buffer buf;
    fc_buffer_init(&buf, 0);
    for (int i = 0; i < WARMUP; i++) {
        fc_buffer_reset(&buf);
        rbp_binary_new_order_req_encode(&msg, &buf);
    }
    double best = 1e18;
    for (int b = 0; b < BATCHES; b++) {
        double t0 = now_ns();
        for (int i = 0; i < ITERS; i++) {
            fc_buffer_reset(&buf);
            rbp_binary_new_order_req_encode(&msg, &buf);
            sink += buf.len;
        }
        double d = now_ns() - t0;
        if (d < best) best = d;
    }
    size_t bytes = buf.len;
    report("encode", best, bytes);
    fc_buffer_free(&buf);
    return bytes;
}

static void bench_decode(const uint8_t *raw, size_t len) {
    uint64_t s = 0;
    for (int i = 0; i < WARMUP; i++) {
        fc_reader r;
        fc_reader_init(&r, raw, len, 0);
        rbp_binary_new_order_req m;
        rbp_binary_new_order_req_decode(&r, &m);
        s += m.transact_time;
        rbp_binary_new_order_req_free(&m);
    }
    double best = 1e18;
    for (int b = 0; b < BATCHES; b++) {
        double t0 = now_ns();
        for (int i = 0; i < ITERS; i++) {
            fc_reader r;
            fc_reader_init(&r, raw, len, 0);
            rbp_binary_new_order_req m;
            rbp_binary_new_order_req_decode(&r, &m);
            s += m.transact_time;
            rbp_binary_new_order_req_free(&m);
        }
        double d = now_ns() - t0;
        if (d < best) best = d;
    }
    sink += s;
    report("decode", best, len);
}

int main(void) {
    bench_encode();

    /* pre-encode one frame for the decode benchmark */
    rbp_binary_new_order_req msg;
    fill_new_order_req(&msg);
    fc_buffer buf;
    fc_buffer_init(&buf, 0);
    if (!rbp_binary_new_order_req_encode(&msg, &buf)) return 1;
    bench_decode(buf.data, buf.len);
    fc_buffer_free(&buf);

    if (sink == 1) fprintf(stderr, "\n");
    return 0;
}
