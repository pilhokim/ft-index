/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
// vim: ft=cpp:expandtab:ts=8:sw=4:softtabstop=4:
#ident "$Id$"
#ident "Copyright (c) 2007-2013 Tokutek Inc.  All rights reserved."
#include "test.h"
/* Try to exercise all the cases for the leafcommands in ft-ops.c
 */


#include <db.h>
#include <sys/stat.h>

static DB_ENV *env;
static DB *db;
static DB_TXN *txn;

static void insert (int i, int j) {
    char hello[30], there[30];
    DBT key,data;
    if (verbose) printf("Insert %d\n", i);
    snprintf(hello, sizeof(hello), "hello%d", i);
    snprintf(there, sizeof(there), "there%d", j);
    int r = db->put(db, txn,
		    dbt_init(&key,  hello, strlen(hello)+1),
		    dbt_init(&data, there, strlen(there)+1),
		    0);
    CKERR(r);
}

static void op_delete (int i) {
    char hello[30];
    DBT key;
    if (verbose) printf("op_delete %d\n", i);
    snprintf(hello, sizeof(hello), "hello%d", i);
    int r = db->del(db, txn,
		    dbt_init(&key,  hello, strlen(hello)+1),
		    DB_DELETE_ANY);
#ifdef TOKUDB
    assert(r==0);
#else
    assert(r==DB_NOTFOUND || r==0);
#endif
}

static void lookup (int i, int expect, int expectj) {
    char hello[30], there[30];
    DBT key,data;
    snprintf(hello, sizeof(hello), "hello%d", i);
    memset(&data, 0, sizeof(data));
    if (verbose) printf("Looking up %d (expecting %s)\n", i, expect==0 ? "to find" : "not to find");
    int r = db->get(db, txn,
		    dbt_init(&key,  hello, strlen(hello)+1),
		    &data,
		    0);
    assert(expect==r);
    if (expect==0) {
	CKERR(r);
	snprintf(there, sizeof(there), "there%d", expectj);
	assert(data.size==strlen(there)+1);
	assert(strcmp((char*)data.data, there)==0);
    }
}

static void
test_abort3 (void) {
    int r;
    toku_os_recursive_delete(TOKU_TEST_FILENAME);
    r=toku_os_mkdir(TOKU_TEST_FILENAME, S_IRWXU+S_IRWXG+S_IRWXO);       assert(r==0);

    r=db_env_create(&env, 0); assert(r==0);
    env->set_errfile(env, stderr);
    r=env->open(env, TOKU_TEST_FILENAME, DB_INIT_LOCK|DB_INIT_LOG|DB_INIT_MPOOL|DB_INIT_TXN|DB_CREATE|DB_PRIVATE, S_IRWXU+S_IRWXG+S_IRWXO); CKERR(r);
    r=db_create(&db, env, 0); CKERR(r);

    r=env->txn_begin(env, 0, &txn, 0); assert(r==0);
    r=db->open(db, txn, "foo.db", 0, DB_BTREE, DB_CREATE, S_IRWXU+S_IRWXG+S_IRWXO); CKERR(r);
    insert(0, 0);
    r=txn->commit(txn, 0);    assert(r==0);

    r=env->txn_begin(env, 0, &txn, 0);    CKERR(r);
    op_delete(0);
    op_delete(1);
    r=txn->commit(txn, 0); CKERR(r);

    r=env->txn_begin(env, 0, &txn, 0);    CKERR(r);    
    lookup(1, DB_NOTFOUND, -1);
    insert(2, 3);
    r=txn->commit(txn, 0); CKERR(r);

    r=env->txn_begin(env, 0, &txn, 0);    CKERR(r);    
    insert(2, 4);
    insert(2, 5);
    lookup(2, 0, 5);
    r=txn->commit(txn, 0); CKERR(r);

    r=env->txn_begin(env, 0, &txn, 0);    CKERR(r);
    lookup(2, 0, 5);
    r=txn->commit(txn, 0); CKERR(r);

    r=env->txn_begin(env, 0, &txn, 0);    CKERR(r);
    insert(3, 0);
    r=txn->commit(txn, 0); CKERR(r);
    
    r=env->txn_begin(env, 0, &txn, 0);    CKERR(r);
    insert(3, 1);
    lookup(3, 0, 1);
    r=txn->abort(txn); CKERR(r);

    r=env->txn_begin(env, 0, &txn, 0);    CKERR(r);    
    lookup(3, 0, 0);
    r=txn->commit(txn, 0); CKERR(r);

    r=env->txn_begin(env, 0, &txn, 0);    CKERR(r);
    insert(4, 0);
    r=txn->commit(txn, 0); CKERR(r);
    
    r=env->txn_begin(env, 0, &txn, 0);    CKERR(r);
    op_delete(4);
    lookup(4, DB_NOTFOUND, -1);
    r=txn->abort(txn); CKERR(r);

    r=env->txn_begin(env, 0, &txn, 0);    CKERR(r);    
    lookup(4, 0, 0);
    r=txn->commit(txn, 0); CKERR(r);


    r=env->txn_begin(env, 0, &txn, 0);    CKERR(r);
    insert(5, 0);
    r=txn->commit(txn, 0); CKERR(r);
    
    r=env->txn_begin(env, 0, &txn, 0);    CKERR(r);
    insert(5, 1);
    lookup(5, 0, 1);
    op_delete(5);
    lookup(5, DB_NOTFOUND, -1);
    r=txn->abort(txn); CKERR(r);

    r=env->txn_begin(env, 0, &txn, 0);    CKERR(r);    
    lookup(5, 0, 0);
    r=txn->commit(txn, 0); CKERR(r);

    r=env->txn_begin(env, 0, &txn, 0);    CKERR(r);
    r=txn->commit(txn, 0); CKERR(r);
    
    r=env->txn_begin(env, 0, &txn, 0);    CKERR(r);
    insert(6, 0);
    lookup(6, 0, 0);
    op_delete(6);
    lookup(6, DB_NOTFOUND, -1);
    r=txn->abort(txn); CKERR(r);

    r=env->txn_begin(env, 0, &txn, 0);    CKERR(r);    
    lookup(6, DB_NOTFOUND, -1);
    r=txn->commit(txn, 0); CKERR(r);


    r=db->close(db, 0); CKERR(r);
    r=env->close(env, 0); CKERR(r);
}

int
test_main (int argc, char *const argv[]) {
    parse_args(argc, argv);
    test_abort3();
    return 0;
}
