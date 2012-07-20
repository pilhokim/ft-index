/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
// vim: ft=cpp:expandtab:ts=8:sw=4:softtabstop=4:
#ident "$Id$"
#ident "Copyright (c) 2007 Tokutek Inc.  All rights reserved."
#include "test.h"
#include <stdio.h>

#include <stdlib.h>
#include <unistd.h>
#include <memory.h>
#include <sys/stat.h>
#include <db.h>

static void
test_txn_abort (void) {

    int r;
    r = system("rm -rf " ENVDIR);
    CKERR(r);
    toku_os_mkdir(ENVDIR, S_IRWXU+S_IRWXG+S_IRWXO);

    int i;
    DB_ENV *env;
    DBT key, val;
    DB_TXN* txn_all = NULL;
    DB_TXN* txn_stmt = NULL;
    DB_TXN* txn_sp = NULL;
    r = db_env_create(&env, 0); CKERR(r);
    r = env->open(env, ENVDIR, DB_INIT_MPOOL | DB_INIT_LOG | DB_INIT_LOCK | DB_INIT_TXN | DB_PRIVATE | DB_CREATE, S_IRWXU+S_IRWXG+S_IRWXO); 
    CKERR(r);

    DB *db = NULL;
    {
        DB_TXN *txn = 0;
        r = env->txn_begin(env, 0, &txn, 0); CKERR(r);

        r = db_create(&db, env, 0); CKERR(r);
        r = db->open(db, txn, "test.db", 0, DB_BTREE, DB_CREATE, S_IRWXU+S_IRWXG+S_IRWXO); CKERR(r);
        r = txn->commit(txn, 0); CKERR(r);
    }


    r = env->txn_begin(env, 0, &txn_all, 0); CKERR(r);

    r = env->txn_begin(env, txn_all, &txn_stmt, 0); CKERR(r);
    i = 1;
    r = db->put(db, txn_stmt, dbt_init(&key, &i, sizeof i), dbt_init(&val, &i, sizeof i), 0); 
    CKERR(r);
    r = txn_stmt->commit(txn_stmt,DB_TXN_NOSYNC); 
    txn_stmt = NULL;
    
    r = env->txn_begin(env, txn_all, &txn_sp, 0); CKERR(r);
    
    r = env->txn_begin(env, txn_sp, &txn_stmt, 0); CKERR(r);
    r = db->del(db, txn_stmt, dbt_init(&key, &i, sizeof i), 0);
    CKERR(r);


    r = txn_stmt->commit(txn_stmt,DB_TXN_NOSYNC); 
    txn_stmt = NULL;


    r = txn_all->abort(txn_all);
    CKERR(r);


    {
        /* walk the db, should be empty */
        DB_TXN *txn = 0;
        r = env->txn_begin(env, 0, &txn, 0); CKERR(r);
        DBC *cursor;
        r = db->cursor(db, txn, &cursor, 0); CKERR(r);
        memset(&key, 0, sizeof key);
        memset(&val, 0, sizeof val);
        r = cursor->c_get(cursor, &key, &val, DB_FIRST); 
        CKERR2(r, DB_NOTFOUND);
        r = cursor->c_close(cursor); CKERR(r);
        r = txn->commit(txn, 0);
    }

    r = db->close(db, 0); CKERR(r);
    r = env->close(env, 0); CKERR(r);
}

int
test_main(int argc, char *const argv[]) {
    parse_args(argc, argv);
    test_txn_abort();
    return 0;
}