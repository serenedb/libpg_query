add_library(pg_query STATIC 
# src
  src/pg_query_deparse.c
  src/pg_query_fingerprint.c
  src/pg_query_json_plpgsql.c
  src/pg_query_normalize.c
  src/pg_query_outfuncs_json.c
  # src/pg_query_outfuncs_protobuf_cpp.cc 
  src/pg_query_outfuncs_protobuf.c
  src/pg_query_parse_plpgsql.c
  src/pg_query_parse.c
  src/pg_query_raw_tree_walker_supports.c
  src/pg_query_readfuncs_protobuf.c
  src/pg_query_scan.c
  src/pg_query_split.c
  src/pg_query_summary_statement_type.c
  src/pg_query_summary_truncate.c
  src/pg_query_summary.c
  src/pg_query.c
  src/postgres_deparse.c
# Other sources
  src/postgres/src_backend_catalog_namespace.c
  src/postgres/src_backend_catalog_pg_proc.c
  src/postgres/src_backend_commands_define.c
  src/postgres/src_backend_nodes_bitmapset.c
  src/postgres/src_backend_nodes_copyfuncs.c
  src/postgres/src_backend_nodes_equalfuncs.c
  src/postgres/src_backend_nodes_extensible.c
  src/postgres/src_backend_nodes_list.c
  src/postgres/src_backend_nodes_makefuncs.c
  src/postgres/src_backend_nodes_nodeFuncs.c
  src/postgres/src_backend_nodes_value.c
  src/postgres/src_backend_parser_gram.c
  src/postgres/src_backend_parser_parse_target.c
  src/postgres/src_backend_parser_parser.c
  src/postgres/src_backend_parser_scan.c
  src/postgres/src_backend_parser_scansup.c
  src/postgres/src_backend_storage_ipc_ipc.c
  src/postgres/src_backend_tcop_cmdtag.c
  src/postgres/src_backend_tcop_postgres.c
  src/postgres/src_backend_tcop_utility.c
  src/postgres/src_backend_utils_activity_pgstat_database.c
  src/postgres/src_backend_utils_adt_datum.c
  src/postgres/src_backend_utils_adt_expandeddatum.c
  src/postgres/src_backend_utils_adt_format_type.c
  src/postgres/src_backend_utils_adt_datetime.cpp
  src/postgres/src_backend_utils_adt_numutils.c
  src/postgres/src_backend_utils_adt_ruleutils.c
  src/postgres/src_backend_utils_adt_timestamp.c
  src/postgres/src_backend_utils_error_assert.c
  src/postgres/src_backend_utils_error_elog.c
  src/postgres/src_backend_utils_fmgr_fmgr.c
  src/postgres/src_backend_utils_init_globals.c
  src/postgres/src_backend_utils_mb_mbutils.c
  src/postgres/src_backend_utils_misc_guc_tables.c
  src/postgres/src_backend_utils_mmgr_alignedalloc.c
  src/postgres/src_backend_utils_mmgr_aset.c
  src/postgres/src_backend_utils_mmgr_bump.c
  src/postgres/src_backend_utils_mmgr_generation.c
  src/postgres/src_backend_utils_mmgr_mcxt.c
  src/postgres/src_backend_utils_mmgr_slab.c
  src/postgres/src_common_encnames.c
  src/postgres/src_common_hashfn.c
  src/postgres/src_common_keywords.c
  src/postgres/src_common_kwlookup.c
  src/postgres/src_common_psprintf.c
  src/postgres/src_common_stringinfo.c
  src/postgres/src_common_string.c
  src/postgres/src_common_wchar.c
  src/postgres/src_pl_plpgsql_src_pl_comp.c
  src/postgres/src_pl_plpgsql_src_pl_funcs.c
  src/postgres/src_pl_plpgsql_src_pl_gram.c
  src/postgres/src_pl_plpgsql_src_pl_handler.c
  src/postgres/src_pl_plpgsql_src_pl_scanner.c
  src/postgres/src_port_pg_bitutils.c
  src/postgres/src_port_pgstrcasecmp.c
  src/postgres/src_port_qsort.c
  src/postgres/src_port_snprintf.c
  src/postgres/src_port_strerror.c
  src/postgres/src_port_strlcpy.c
)

if (SDB_DEV)
  target_compile_definitions(pg_query PRIVATE USE_ASSERT_CHECKING)
endif()

target_include_directories(pg_query
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/include
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/postgres/include
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/vendor
)

target_compile_options(pg_query PRIVATE -w)
