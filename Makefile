all: nntpdb

nntpdb: common.c main.c nntp_db.c nntp_srv.c ../vdb/vsqlite.c ../vdb/sqlite3/sqlite3.c
		$(CC) -o nntpdb -I../vos -I ../vdb main.c common.c nntp_db.c nntp_srv.c \
		../vdb/vsqlite.c ../vdb/sqlite3/sqlite3.c \
		-lpthread -ldl