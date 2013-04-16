/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
// vim: ft=cpp:expandtab:ts=8:sw=4:softtabstop=4:
#ifndef TEST_FT_TXNS_H
#define TEST_FT_TXNS_H

#ident "$Id$"
#ident "Copyright (c) 2010-2013 Tokutek Inc.  All rights reserved."
#ident "The technology is licensed by the Massachusetts Institute of Technology, Rutgers State University of New Jersey, and the Research Foundation of State University of New York at Stony Brook under United States of America Serial No. 11/760379 and to the patents and/or patent applications resulting from it."

static inline void
test_setup(const char *envdir, TOKULOGGER *loggerp, CACHETABLE *ctp) {
    *loggerp = NULL;
    *ctp = NULL;
    int r;
    toku_os_recursive_delete(envdir);
    r = toku_os_mkdir(envdir, S_IRWXU);
    CKERR(r);

    r = toku_logger_create(loggerp);
    CKERR(r);
    TOKULOGGER logger = *loggerp;

    r = toku_logger_open(envdir, logger);
    CKERR(r);

    toku_cachetable_create(ctp, 0, ZERO_LSN, logger);
    CACHETABLE ct = *ctp;
    toku_cachetable_set_env_dir(ct, envdir);

    toku_logger_set_cachetable(logger, ct);

    r = toku_logger_open_rollback(logger, ct, true);
    CKERR(r);

    CHECKPOINTER cp = toku_cachetable_get_checkpointer(*ctp);
    r = toku_checkpoint(cp, logger, NULL, NULL, NULL, NULL, STARTUP_CHECKPOINT);
    CKERR(r);
}

static inline void
xid_lsn_keep_cachetable_callback (DB_ENV *env, CACHETABLE cachetable) {
    CACHETABLE *CAST_FROM_VOIDP(ctp, (void *) env);
    *ctp = cachetable;
}

static inline void test_setup_and_recover(const char *envdir, TOKULOGGER *loggerp, CACHETABLE *ctp) {
    int r;
    TOKULOGGER logger = NULL;
    CACHETABLE ct = NULL;
    r = toku_logger_create(&logger);
    CKERR(r);

    DB_ENV *CAST_FROM_VOIDP(ctv, (void *) &ct);  // Use intermediate to avoid compiler warning.
    r = tokudb_recover(ctv,
                       NULL_prepared_txn_callback,
                       xid_lsn_keep_cachetable_callback,
                       logger,
                       envdir, envdir, 0, 0, 0, NULL, 0);
    CKERR(r);
    if (!toku_logger_is_open(logger)) {
        //Did not need recovery.
        invariant(ct==NULL);
        r = toku_logger_open(envdir, logger);
        CKERR(r);
        toku_cachetable_create(&ct, 0, ZERO_LSN, logger);
        toku_logger_set_cachetable(logger, ct);
    }
    *ctp = ct;
    *loggerp = logger;
}

static inline void clean_shutdown(TOKULOGGER *loggerp, CACHETABLE *ctp) {
    int r;
    CHECKPOINTER cp = toku_cachetable_get_checkpointer(*ctp);
    r = toku_checkpoint(cp, *loggerp, NULL, NULL, NULL, NULL, SHUTDOWN_CHECKPOINT);
    CKERR(r);

    toku_logger_close_rollback(*loggerp);

    r = toku_checkpoint(cp, *loggerp, NULL, NULL, NULL, NULL, SHUTDOWN_CHECKPOINT);
    CKERR(r);

    toku_logger_shutdown(*loggerp);

    toku_cachetable_close(ctp);

    r = toku_logger_close(loggerp);
    CKERR(r);
}

static inline void shutdown_after_recovery(TOKULOGGER *loggerp, CACHETABLE *ctp) {
    toku_logger_close_rollback(*loggerp);
    toku_cachetable_close(ctp);
    int r = toku_logger_close(loggerp);
    CKERR(r);
}

#endif /* TEST_FT_TXNS_H */
