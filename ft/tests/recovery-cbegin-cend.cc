/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
// vim: ft=cpp:expandtab:ts=8:sw=4:softtabstop=4:
#ident "$Id$"
#ident "Copyright (c) 2007-2013 Tokutek Inc.  All rights reserved."
#ident "The technology is licensed by the Massachusetts Institute of Technology, Rutgers State University of New Jersey, and the Research Foundation of State University of New York at Stony Brook under United States of America Serial No. 11/760379 and to the patents and/or patent applications resulting from it."
#include "test.h"


static int 
run_test(void) {
    int r;

    // setup the test dir
    toku_os_recursive_delete(TOKU_TEST_FILENAME);
    r = toku_os_mkdir(TOKU_TEST_FILENAME, S_IRWXU); assert(r == 0);

    // put begin and end checkpoint into separate log files
    TOKULOGGER logger;
    r = toku_logger_create(&logger); assert(r == 0);
    r = toku_logger_open(TOKU_TEST_FILENAME, logger); assert(r == 0);
    LSN beginlsn;
    toku_log_begin_checkpoint(logger, &beginlsn, false, 0, 0);
    r = toku_logger_close(&logger); assert(r == 0);

    r = toku_logger_create(&logger); assert(r == 0);
    r = toku_logger_open(TOKU_TEST_FILENAME, logger); assert(r == 0);
    toku_log_end_checkpoint(logger, NULL, true, beginlsn, 0, 0, 0);
    r = toku_logger_close(&logger); assert(r == 0);

    // run recovery
    r = tokudb_recover(NULL,
		       NULL_prepared_txn_callback,
		       NULL_keep_cachetable_callback,
		       NULL_logger, TOKU_TEST_FILENAME, TOKU_TEST_FILENAME, 0, 0, 0, NULL, 0); 
    assert(r == 0);

    toku_os_recursive_delete(TOKU_TEST_FILENAME);

    return 0;
}

int
test_main(int UU(argc), const char *UU(argv[])) {
    int r;
    r = run_test();
    return r;
}
