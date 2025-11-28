// clang-format off
/*-------------------------------------------------------------------------
 *
 * utility.c
 *	  Contains functions which control the execution of the POSTGRES utility
 *	  commands.  At one time acted as an interface between the Lisp and C
 *	  systems.
 *
 * Portions Copyright (c) 1996-2025, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  src/backend/tcop/utility.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "tcop/utility.h"

/*
 * AlterObjectTypeCommandTag
 *		helper function for CreateCommandTag
 *
 * This covers most cases where ALTER is used with an ObjectType enum.
 */
static CommandTag
AlterObjectTypeCommandTag(ObjectType objtype)
{
	CommandTag	tag;

	switch (objtype)
	{
		case OBJECT_AGGREGATE:
			tag = CMDTAG_ALTER_AGGREGATE;
			break;
		case OBJECT_ATTRIBUTE:
			tag = CMDTAG_ALTER_TYPE;
			break;
		case OBJECT_CAST:
			tag = CMDTAG_ALTER_CAST;
			break;
		case OBJECT_COLLATION:
			tag = CMDTAG_ALTER_COLLATION;
			break;
		case OBJECT_COLUMN:
			tag = CMDTAG_ALTER_TABLE;
			break;
		case OBJECT_CONVERSION:
			tag = CMDTAG_ALTER_CONVERSION;
			break;
		case OBJECT_DATABASE:
			tag = CMDTAG_ALTER_DATABASE;
			break;
		case OBJECT_DOMAIN:
		case OBJECT_DOMCONSTRAINT:
			tag = CMDTAG_ALTER_DOMAIN;
			break;
		case OBJECT_EXTENSION:
			tag = CMDTAG_ALTER_EXTENSION;
			break;
		case OBJECT_FDW:
			tag = CMDTAG_ALTER_FOREIGN_DATA_WRAPPER;
			break;
		case OBJECT_FOREIGN_SERVER:
			tag = CMDTAG_ALTER_SERVER;
			break;
		case OBJECT_FOREIGN_TABLE:
			tag = CMDTAG_ALTER_FOREIGN_TABLE;
			break;
		case OBJECT_FUNCTION:
			tag = CMDTAG_ALTER_FUNCTION;
			break;
		case OBJECT_INDEX:
			tag = CMDTAG_ALTER_INDEX;
			break;
		case OBJECT_LANGUAGE:
			tag = CMDTAG_ALTER_LANGUAGE;
			break;
		case OBJECT_LARGEOBJECT:
			tag = CMDTAG_ALTER_LARGE_OBJECT;
			break;
		case OBJECT_OPCLASS:
			tag = CMDTAG_ALTER_OPERATOR_CLASS;
			break;
		case OBJECT_OPERATOR:
			tag = CMDTAG_ALTER_OPERATOR;
			break;
		case OBJECT_OPFAMILY:
			tag = CMDTAG_ALTER_OPERATOR_FAMILY;
			break;
		case OBJECT_POLICY:
			tag = CMDTAG_ALTER_POLICY;
			break;
		case OBJECT_PROCEDURE:
			tag = CMDTAG_ALTER_PROCEDURE;
			break;
		case OBJECT_ROLE:
			tag = CMDTAG_ALTER_ROLE;
			break;
		case OBJECT_ROUTINE:
			tag = CMDTAG_ALTER_ROUTINE;
			break;
		case OBJECT_RULE:
			tag = CMDTAG_ALTER_RULE;
			break;
		case OBJECT_SCHEMA:
			tag = CMDTAG_ALTER_SCHEMA;
			break;
		case OBJECT_SEQUENCE:
			tag = CMDTAG_ALTER_SEQUENCE;
			break;
		case OBJECT_TABLE:
		case OBJECT_TABCONSTRAINT:
			tag = CMDTAG_ALTER_TABLE;
			break;
		case OBJECT_TABLESPACE:
			tag = CMDTAG_ALTER_TABLESPACE;
			break;
		case OBJECT_TRIGGER:
			tag = CMDTAG_ALTER_TRIGGER;
			break;
		case OBJECT_EVENT_TRIGGER:
			tag = CMDTAG_ALTER_EVENT_TRIGGER;
			break;
		case OBJECT_TSCONFIGURATION:
			tag = CMDTAG_ALTER_TEXT_SEARCH_CONFIGURATION;
			break;
		case OBJECT_TSDICTIONARY:
			tag = CMDTAG_ALTER_TEXT_SEARCH_DICTIONARY;
			break;
		case OBJECT_TSPARSER:
			tag = CMDTAG_ALTER_TEXT_SEARCH_PARSER;
			break;
		case OBJECT_TSTEMPLATE:
			tag = CMDTAG_ALTER_TEXT_SEARCH_TEMPLATE;
			break;
		case OBJECT_TYPE:
			tag = CMDTAG_ALTER_TYPE;
			break;
		case OBJECT_VIEW:
			tag = CMDTAG_ALTER_VIEW;
			break;
		case OBJECT_MATVIEW:
			tag = CMDTAG_ALTER_MATERIALIZED_VIEW;
			break;
		case OBJECT_PUBLICATION:
			tag = CMDTAG_ALTER_PUBLICATION;
			break;
		case OBJECT_SUBSCRIPTION:
			tag = CMDTAG_ALTER_SUBSCRIPTION;
			break;
		case OBJECT_STATISTIC_EXT:
			tag = CMDTAG_ALTER_STATISTICS;
			break;
		default:
			tag = CMDTAG_UNKNOWN;
			break;
	}

	return tag;
}

/*
 * CreateCommandTag
 *		utility to get a CommandTag for the command operation,
 *		given either a raw (un-analyzed) parsetree, an analyzed Query,
 *		or a PlannedStmt.
 *
 * This must handle all command types, but since the vast majority
 * of 'em are utility commands, it seems sensible to keep it here.
 */
CommandTag
CreateCommandTag(Node *parsetree)
{
	CommandTag	tag;

	switch (nodeTag(parsetree))
	{
			/* recurse if we're given a RawStmt */
		case T_RawStmt:
			tag = CreateCommandTag(((RawStmt *) parsetree)->stmt);
			break;

			/* raw plannable queries */
		case T_InsertStmt:
			tag = CMDTAG_INSERT;
			break;

		case T_DeleteStmt:
			tag = CMDTAG_DELETE;
			break;

		case T_UpdateStmt:
			tag = CMDTAG_UPDATE;
			break;

		case T_MergeStmt:
			tag = CMDTAG_MERGE;
			break;

		case T_SelectStmt:
			tag = CMDTAG_SELECT;
			break;

		case T_PLAssignStmt:
			tag = CMDTAG_SELECT;
			break;

			/* utility statements --- same whether raw or cooked */
		case T_TransactionStmt:
			{
				TransactionStmt *stmt = (TransactionStmt *) parsetree;

				switch (stmt->kind)
				{
					case TRANS_STMT_BEGIN:
						tag = CMDTAG_BEGIN;
						break;

					case TRANS_STMT_START:
						tag = CMDTAG_START_TRANSACTION;
						break;

					case TRANS_STMT_COMMIT:
						tag = CMDTAG_COMMIT;
						break;

					case TRANS_STMT_ROLLBACK:
					case TRANS_STMT_ROLLBACK_TO:
						tag = CMDTAG_ROLLBACK;
						break;

					case TRANS_STMT_SAVEPOINT:
						tag = CMDTAG_SAVEPOINT;
						break;

					case TRANS_STMT_RELEASE:
						tag = CMDTAG_RELEASE;
						break;

					case TRANS_STMT_PREPARE:
						tag = CMDTAG_PREPARE_TRANSACTION;
						break;

					case TRANS_STMT_COMMIT_PREPARED:
						tag = CMDTAG_COMMIT_PREPARED;
						break;

					case TRANS_STMT_ROLLBACK_PREPARED:
						tag = CMDTAG_ROLLBACK_PREPARED;
						break;

					default:
						tag = CMDTAG_UNKNOWN;
						break;
				}
			}
			break;

		case T_DeclareCursorStmt:
			tag = CMDTAG_DECLARE_CURSOR;
			break;

		case T_ClosePortalStmt:
			{
				ClosePortalStmt *stmt = (ClosePortalStmt *) parsetree;

				if (stmt->portalname == NULL)
					tag = CMDTAG_CLOSE_CURSOR_ALL;
				else
					tag = CMDTAG_CLOSE_CURSOR;
			}
			break;

		case T_FetchStmt:
			{
				FetchStmt  *stmt = (FetchStmt *) parsetree;

				tag = (stmt->ismove) ? CMDTAG_MOVE : CMDTAG_FETCH;
			}
			break;

		case T_CreateDomainStmt:
			tag = CMDTAG_CREATE_DOMAIN;
			break;

		case T_CreateSchemaStmt:
			tag = CMDTAG_CREATE_SCHEMA;
			break;

		case T_CreateStmt:
			tag = CMDTAG_CREATE_TABLE;
			break;

		case T_CreateTableSpaceStmt:
			tag = CMDTAG_CREATE_TABLESPACE;
			break;

		case T_DropTableSpaceStmt:
			tag = CMDTAG_DROP_TABLESPACE;
			break;

		case T_AlterTableSpaceOptionsStmt:
			tag = CMDTAG_ALTER_TABLESPACE;
			break;

		case T_CreateExtensionStmt:
			tag = CMDTAG_CREATE_EXTENSION;
			break;

		case T_AlterExtensionStmt:
			tag = CMDTAG_ALTER_EXTENSION;
			break;

		case T_AlterExtensionContentsStmt:
			tag = CMDTAG_ALTER_EXTENSION;
			break;

		case T_CreateFdwStmt:
			tag = CMDTAG_CREATE_FOREIGN_DATA_WRAPPER;
			break;

		case T_AlterFdwStmt:
			tag = CMDTAG_ALTER_FOREIGN_DATA_WRAPPER;
			break;

		case T_CreateForeignServerStmt:
			tag = CMDTAG_CREATE_SERVER;
			break;

		case T_AlterForeignServerStmt:
			tag = CMDTAG_ALTER_SERVER;
			break;

		case T_CreateUserMappingStmt:
			tag = CMDTAG_CREATE_USER_MAPPING;
			break;

		case T_AlterUserMappingStmt:
			tag = CMDTAG_ALTER_USER_MAPPING;
			break;

		case T_DropUserMappingStmt:
			tag = CMDTAG_DROP_USER_MAPPING;
			break;

		case T_CreateForeignTableStmt:
			tag = CMDTAG_CREATE_FOREIGN_TABLE;
			break;

		case T_ImportForeignSchemaStmt:
			tag = CMDTAG_IMPORT_FOREIGN_SCHEMA;
			break;

		case T_DropStmt:
			switch (((DropStmt *) parsetree)->removeType)
			{
				case OBJECT_TABLE:
					tag = CMDTAG_DROP_TABLE;
					break;
				case OBJECT_SEQUENCE:
					tag = CMDTAG_DROP_SEQUENCE;
					break;
				case OBJECT_VIEW:
					tag = CMDTAG_DROP_VIEW;
					break;
				case OBJECT_MATVIEW:
					tag = CMDTAG_DROP_MATERIALIZED_VIEW;
					break;
				case OBJECT_INDEX:
					tag = CMDTAG_DROP_INDEX;
					break;
				case OBJECT_TYPE:
					tag = CMDTAG_DROP_TYPE;
					break;
				case OBJECT_DOMAIN:
					tag = CMDTAG_DROP_DOMAIN;
					break;
				case OBJECT_COLLATION:
					tag = CMDTAG_DROP_COLLATION;
					break;
				case OBJECT_CONVERSION:
					tag = CMDTAG_DROP_CONVERSION;
					break;
				case OBJECT_SCHEMA:
					tag = CMDTAG_DROP_SCHEMA;
					break;
				case OBJECT_TSPARSER:
					tag = CMDTAG_DROP_TEXT_SEARCH_PARSER;
					break;
				case OBJECT_TSDICTIONARY:
					tag = CMDTAG_DROP_TEXT_SEARCH_DICTIONARY;
					break;
				case OBJECT_TSTEMPLATE:
					tag = CMDTAG_DROP_TEXT_SEARCH_TEMPLATE;
					break;
				case OBJECT_TSCONFIGURATION:
					tag = CMDTAG_DROP_TEXT_SEARCH_CONFIGURATION;
					break;
				case OBJECT_FOREIGN_TABLE:
					tag = CMDTAG_DROP_FOREIGN_TABLE;
					break;
				case OBJECT_EXTENSION:
					tag = CMDTAG_DROP_EXTENSION;
					break;
				case OBJECT_FUNCTION:
					tag = CMDTAG_DROP_FUNCTION;
					break;
				case OBJECT_PROCEDURE:
					tag = CMDTAG_DROP_PROCEDURE;
					break;
				case OBJECT_ROUTINE:
					tag = CMDTAG_DROP_ROUTINE;
					break;
				case OBJECT_AGGREGATE:
					tag = CMDTAG_DROP_AGGREGATE;
					break;
				case OBJECT_OPERATOR:
					tag = CMDTAG_DROP_OPERATOR;
					break;
				case OBJECT_LANGUAGE:
					tag = CMDTAG_DROP_LANGUAGE;
					break;
				case OBJECT_CAST:
					tag = CMDTAG_DROP_CAST;
					break;
				case OBJECT_TRIGGER:
					tag = CMDTAG_DROP_TRIGGER;
					break;
				case OBJECT_EVENT_TRIGGER:
					tag = CMDTAG_DROP_EVENT_TRIGGER;
					break;
				case OBJECT_RULE:
					tag = CMDTAG_DROP_RULE;
					break;
				case OBJECT_FDW:
					tag = CMDTAG_DROP_FOREIGN_DATA_WRAPPER;
					break;
				case OBJECT_FOREIGN_SERVER:
					tag = CMDTAG_DROP_SERVER;
					break;
				case OBJECT_OPCLASS:
					tag = CMDTAG_DROP_OPERATOR_CLASS;
					break;
				case OBJECT_OPFAMILY:
					tag = CMDTAG_DROP_OPERATOR_FAMILY;
					break;
				case OBJECT_POLICY:
					tag = CMDTAG_DROP_POLICY;
					break;
				case OBJECT_TRANSFORM:
					tag = CMDTAG_DROP_TRANSFORM;
					break;
				case OBJECT_ACCESS_METHOD:
					tag = CMDTAG_DROP_ACCESS_METHOD;
					break;
				case OBJECT_PUBLICATION:
					tag = CMDTAG_DROP_PUBLICATION;
					break;
				case OBJECT_STATISTIC_EXT:
					tag = CMDTAG_DROP_STATISTICS;
					break;
				default:
					tag = CMDTAG_UNKNOWN;
			}
			break;

		case T_TruncateStmt:
			tag = CMDTAG_TRUNCATE_TABLE;
			break;

		case T_CommentStmt:
			tag = CMDTAG_COMMENT;
			break;

		case T_SecLabelStmt:
			tag = CMDTAG_SECURITY_LABEL;
			break;

		case T_CopyStmt:
			tag = CMDTAG_COPY;
			break;

		case T_RenameStmt:

			/*
			 * When the column is renamed, the command tag is created from its
			 * relation type
			 */
			tag = AlterObjectTypeCommandTag(((RenameStmt *) parsetree)->renameType == OBJECT_COLUMN ?
											((RenameStmt *) parsetree)->relationType :
											((RenameStmt *) parsetree)->renameType);
			break;

		case T_AlterObjectDependsStmt:
			tag = AlterObjectTypeCommandTag(((AlterObjectDependsStmt *) parsetree)->objectType);
			break;

		case T_AlterObjectSchemaStmt:
			tag = AlterObjectTypeCommandTag(((AlterObjectSchemaStmt *) parsetree)->objectType);
			break;

		case T_AlterOwnerStmt:
			tag = AlterObjectTypeCommandTag(((AlterOwnerStmt *) parsetree)->objectType);
			break;

		case T_AlterTableMoveAllStmt:
			tag = AlterObjectTypeCommandTag(((AlterTableMoveAllStmt *) parsetree)->objtype);
			break;

		case T_AlterTableStmt:
			tag = AlterObjectTypeCommandTag(((AlterTableStmt *) parsetree)->objtype);
			break;

		case T_AlterDomainStmt:
			tag = CMDTAG_ALTER_DOMAIN;
			break;

		case T_AlterFunctionStmt:
			switch (((AlterFunctionStmt *) parsetree)->objtype)
			{
				case OBJECT_FUNCTION:
					tag = CMDTAG_ALTER_FUNCTION;
					break;
				case OBJECT_PROCEDURE:
					tag = CMDTAG_ALTER_PROCEDURE;
					break;
				case OBJECT_ROUTINE:
					tag = CMDTAG_ALTER_ROUTINE;
					break;
				default:
					tag = CMDTAG_UNKNOWN;
			}
			break;

		case T_GrantStmt:
			{
				GrantStmt  *stmt = (GrantStmt *) parsetree;

				tag = (stmt->is_grant) ? CMDTAG_GRANT : CMDTAG_REVOKE;
			}
			break;

		case T_GrantRoleStmt:
			{
				GrantRoleStmt *stmt = (GrantRoleStmt *) parsetree;

				tag = (stmt->is_grant) ? CMDTAG_GRANT_ROLE : CMDTAG_REVOKE_ROLE;
			}
			break;

		case T_AlterDefaultPrivilegesStmt:
			tag = CMDTAG_ALTER_DEFAULT_PRIVILEGES;
			break;

		case T_DefineStmt:
			switch (((DefineStmt *) parsetree)->kind)
			{
				case OBJECT_AGGREGATE:
					tag = CMDTAG_CREATE_AGGREGATE;
					break;
				case OBJECT_OPERATOR:
					tag = CMDTAG_CREATE_OPERATOR;
					break;
				case OBJECT_TYPE:
					tag = CMDTAG_CREATE_TYPE;
					break;
				case OBJECT_TSPARSER:
					tag = CMDTAG_CREATE_TEXT_SEARCH_PARSER;
					break;
				case OBJECT_TSDICTIONARY:
					tag = CMDTAG_CREATE_TEXT_SEARCH_DICTIONARY;
					break;
				case OBJECT_TSTEMPLATE:
					tag = CMDTAG_CREATE_TEXT_SEARCH_TEMPLATE;
					break;
				case OBJECT_TSCONFIGURATION:
					tag = CMDTAG_CREATE_TEXT_SEARCH_CONFIGURATION;
					break;
				case OBJECT_COLLATION:
					tag = CMDTAG_CREATE_COLLATION;
					break;
				case OBJECT_ACCESS_METHOD:
					tag = CMDTAG_CREATE_ACCESS_METHOD;
					break;
				default:
					tag = CMDTAG_UNKNOWN;
			}
			break;

		case T_CompositeTypeStmt:
			tag = CMDTAG_CREATE_TYPE;
			break;

		case T_CreateEnumStmt:
			tag = CMDTAG_CREATE_TYPE;
			break;

		case T_CreateRangeStmt:
			tag = CMDTAG_CREATE_TYPE;
			break;

		case T_AlterEnumStmt:
			tag = CMDTAG_ALTER_TYPE;
			break;

		case T_ViewStmt:
			tag = CMDTAG_CREATE_VIEW;
			break;

		case T_CreateFunctionStmt:
			if (((CreateFunctionStmt *) parsetree)->is_procedure)
				tag = CMDTAG_CREATE_PROCEDURE;
			else
				tag = CMDTAG_CREATE_FUNCTION;
			break;

		case T_IndexStmt:
			tag = CMDTAG_CREATE_INDEX;
			break;

		case T_RuleStmt:
			tag = CMDTAG_CREATE_RULE;
			break;

		case T_CreateSeqStmt:
			tag = CMDTAG_CREATE_SEQUENCE;
			break;

		case T_AlterSeqStmt:
			tag = CMDTAG_ALTER_SEQUENCE;
			break;

		case T_DoStmt:
			tag = CMDTAG_DO;
			break;

		case T_CreatedbStmt:
			tag = CMDTAG_CREATE_DATABASE;
			break;

		case T_AlterDatabaseStmt:
		case T_AlterDatabaseRefreshCollStmt:
		case T_AlterDatabaseSetStmt:
			tag = CMDTAG_ALTER_DATABASE;
			break;

		case T_DropdbStmt:
			tag = CMDTAG_DROP_DATABASE;
			break;

		case T_NotifyStmt:
			tag = CMDTAG_NOTIFY;
			break;

		case T_ListenStmt:
			tag = CMDTAG_LISTEN;
			break;

		case T_UnlistenStmt:
			tag = CMDTAG_UNLISTEN;
			break;

		case T_LoadStmt:
			tag = CMDTAG_LOAD;
			break;

		case T_CallStmt:
			tag = CMDTAG_CALL;
			break;

		case T_ClusterStmt:
			tag = CMDTAG_CLUSTER;
			break;

		case T_VacuumStmt:
			if (((VacuumStmt *) parsetree)->is_vacuumcmd)
				tag = CMDTAG_VACUUM;
			else
				tag = CMDTAG_ANALYZE;
			break;

		case T_ExplainStmt:
			tag = CMDTAG_EXPLAIN;
			break;

		case T_CreateTableAsStmt:
			switch (((CreateTableAsStmt *) parsetree)->objtype)
			{
				case OBJECT_TABLE:
					if (((CreateTableAsStmt *) parsetree)->is_select_into)
						tag = CMDTAG_SELECT_INTO;
					else
						tag = CMDTAG_CREATE_TABLE_AS;
					break;
				case OBJECT_MATVIEW:
					tag = CMDTAG_CREATE_MATERIALIZED_VIEW;
					break;
				default:
					tag = CMDTAG_UNKNOWN;
			}
			break;

		case T_RefreshMatViewStmt:
			tag = CMDTAG_REFRESH_MATERIALIZED_VIEW;
			break;

		case T_AlterSystemStmt:
			tag = CMDTAG_ALTER_SYSTEM;
			break;

		case T_VariableSetStmt:
			switch (((VariableSetStmt *) parsetree)->kind)
			{
				case VAR_SET_VALUE:
				case VAR_SET_CURRENT:
				case VAR_SET_DEFAULT:
				case VAR_SET_MULTI:
					tag = CMDTAG_SET;
					break;
				case VAR_RESET:
				case VAR_RESET_ALL:
					tag = CMDTAG_RESET;
					break;
				default:
					tag = CMDTAG_UNKNOWN;
			}
			break;

		case T_VariableShowStmt:
			tag = CMDTAG_SHOW;
			break;

		case T_DiscardStmt:
			switch (((DiscardStmt *) parsetree)->target)
			{
				case DISCARD_ALL:
					tag = CMDTAG_DISCARD_ALL;
					break;
				case DISCARD_PLANS:
					tag = CMDTAG_DISCARD_PLANS;
					break;
				case DISCARD_TEMP:
					tag = CMDTAG_DISCARD_TEMP;
					break;
				case DISCARD_SEQUENCES:
					tag = CMDTAG_DISCARD_SEQUENCES;
					break;
				default:
					tag = CMDTAG_UNKNOWN;
			}
			break;

		case T_CreateTransformStmt:
			tag = CMDTAG_CREATE_TRANSFORM;
			break;

		case T_CreateTrigStmt:
			tag = CMDTAG_CREATE_TRIGGER;
			break;

		case T_CreateEventTrigStmt:
			tag = CMDTAG_CREATE_EVENT_TRIGGER;
			break;

		case T_AlterEventTrigStmt:
			tag = CMDTAG_ALTER_EVENT_TRIGGER;
			break;

		case T_CreatePLangStmt:
			tag = CMDTAG_CREATE_LANGUAGE;
			break;

		case T_CreateRoleStmt:
			tag = CMDTAG_CREATE_ROLE;
			break;

		case T_AlterRoleStmt:
			tag = CMDTAG_ALTER_ROLE;
			break;

		case T_AlterRoleSetStmt:
			tag = CMDTAG_ALTER_ROLE;
			break;

		case T_DropRoleStmt:
			tag = CMDTAG_DROP_ROLE;
			break;

		case T_DropOwnedStmt:
			tag = CMDTAG_DROP_OWNED;
			break;

		case T_ReassignOwnedStmt:
			tag = CMDTAG_REASSIGN_OWNED;
			break;

		case T_LockStmt:
			tag = CMDTAG_LOCK_TABLE;
			break;

		case T_ConstraintsSetStmt:
			tag = CMDTAG_SET_CONSTRAINTS;
			break;

		case T_CheckPointStmt:
			tag = CMDTAG_CHECKPOINT;
			break;

		case T_ReindexStmt:
			tag = CMDTAG_REINDEX;
			break;

		case T_CreateConversionStmt:
			tag = CMDTAG_CREATE_CONVERSION;
			break;

		case T_CreateCastStmt:
			tag = CMDTAG_CREATE_CAST;
			break;

		case T_CreateOpClassStmt:
			tag = CMDTAG_CREATE_OPERATOR_CLASS;
			break;

		case T_CreateOpFamilyStmt:
			tag = CMDTAG_CREATE_OPERATOR_FAMILY;
			break;

		case T_AlterOpFamilyStmt:
			tag = CMDTAG_ALTER_OPERATOR_FAMILY;
			break;

		case T_AlterOperatorStmt:
			tag = CMDTAG_ALTER_OPERATOR;
			break;

		case T_AlterTypeStmt:
			tag = CMDTAG_ALTER_TYPE;
			break;

		case T_AlterTSDictionaryStmt:
			tag = CMDTAG_ALTER_TEXT_SEARCH_DICTIONARY;
			break;

		case T_AlterTSConfigurationStmt:
			tag = CMDTAG_ALTER_TEXT_SEARCH_CONFIGURATION;
			break;

		case T_CreatePolicyStmt:
			tag = CMDTAG_CREATE_POLICY;
			break;

		case T_AlterPolicyStmt:
			tag = CMDTAG_ALTER_POLICY;
			break;

		case T_CreateAmStmt:
			tag = CMDTAG_CREATE_ACCESS_METHOD;
			break;

		case T_CreatePublicationStmt:
			tag = CMDTAG_CREATE_PUBLICATION;
			break;

		case T_AlterPublicationStmt:
			tag = CMDTAG_ALTER_PUBLICATION;
			break;

		case T_CreateSubscriptionStmt:
			tag = CMDTAG_CREATE_SUBSCRIPTION;
			break;

		case T_AlterSubscriptionStmt:
			tag = CMDTAG_ALTER_SUBSCRIPTION;
			break;

		case T_DropSubscriptionStmt:
			tag = CMDTAG_DROP_SUBSCRIPTION;
			break;

		case T_AlterCollationStmt:
			tag = CMDTAG_ALTER_COLLATION;
			break;

		case T_PrepareStmt:
			tag = CMDTAG_PREPARE;
			break;

		case T_ExecuteStmt:
			tag = CMDTAG_EXECUTE;
			break;

		case T_CreateStatsStmt:
			tag = CMDTAG_CREATE_STATISTICS;
			break;

		case T_AlterStatsStmt:
			tag = CMDTAG_ALTER_STATISTICS;
			break;

		case T_DeallocateStmt:
			{
				DeallocateStmt *stmt = (DeallocateStmt *) parsetree;

				if (stmt->name == NULL)
					tag = CMDTAG_DEALLOCATE_ALL;
				else
					tag = CMDTAG_DEALLOCATE;
			}
			break;

// pg 18
//		case T_WaitStmt:
//			tag = CMDTAG_WAIT;
//			break;

			/* already-planned queries */
		case T_PlannedStmt:
			{
				PlannedStmt *stmt = (PlannedStmt *) parsetree;

				switch (stmt->commandType)
				{
					case CMD_SELECT:

						/*
						 * We take a little extra care here so that the result
						 * will be useful for complaints about read-only
						 * statements
						 */
						if (stmt->rowMarks != NIL)
						{
							/* not 100% but probably close enough */
							switch (((PlanRowMark *) linitial(stmt->rowMarks))->strength)
							{
								case LCS_FORKEYSHARE:
									tag = CMDTAG_SELECT_FOR_KEY_SHARE;
									break;
								case LCS_FORSHARE:
									tag = CMDTAG_SELECT_FOR_SHARE;
									break;
								case LCS_FORNOKEYUPDATE:
									tag = CMDTAG_SELECT_FOR_NO_KEY_UPDATE;
									break;
								case LCS_FORUPDATE:
									tag = CMDTAG_SELECT_FOR_UPDATE;
									break;
								default:
									tag = CMDTAG_SELECT;
									break;
							}
						}
						else
							tag = CMDTAG_SELECT;
						break;
					case CMD_UPDATE:
						tag = CMDTAG_UPDATE;
						break;
					case CMD_INSERT:
						tag = CMDTAG_INSERT;
						break;
					case CMD_DELETE:
						tag = CMDTAG_DELETE;
						break;
					case CMD_MERGE:
						tag = CMDTAG_MERGE;
						break;
					case CMD_UTILITY:
						tag = CreateCommandTag(stmt->utilityStmt);
						break;
					default:
						elog(WARNING, "unrecognized commandType: %d",
							 (int) stmt->commandType);
						tag = CMDTAG_UNKNOWN;
						break;
				}
			}
			break;

			/* parsed-and-rewritten-but-not-planned queries */
		case T_Query:
			{
				Query	   *stmt = (Query *) parsetree;

				switch (stmt->commandType)
				{
					case CMD_SELECT:

						/*
						 * We take a little extra care here so that the result
						 * will be useful for complaints about read-only
						 * statements
						 */
						if (stmt->rowMarks != NIL)
						{
							/* not 100% but probably close enough */
							switch (((RowMarkClause *) linitial(stmt->rowMarks))->strength)
							{
								case LCS_FORKEYSHARE:
									tag = CMDTAG_SELECT_FOR_KEY_SHARE;
									break;
								case LCS_FORSHARE:
									tag = CMDTAG_SELECT_FOR_SHARE;
									break;
								case LCS_FORNOKEYUPDATE:
									tag = CMDTAG_SELECT_FOR_NO_KEY_UPDATE;
									break;
								case LCS_FORUPDATE:
									tag = CMDTAG_SELECT_FOR_UPDATE;
									break;
								default:
									tag = CMDTAG_UNKNOWN;
									break;
							}
						}
						else
							tag = CMDTAG_SELECT;
						break;
					case CMD_UPDATE:
						tag = CMDTAG_UPDATE;
						break;
					case CMD_INSERT:
						tag = CMDTAG_INSERT;
						break;
					case CMD_DELETE:
						tag = CMDTAG_DELETE;
						break;
					case CMD_MERGE:
						tag = CMDTAG_MERGE;
						break;
					case CMD_UTILITY:
						tag = CreateCommandTag(stmt->utilityStmt);
						break;
					default:
						elog(WARNING, "unrecognized commandType: %d",
							 (int) stmt->commandType);
						tag = CMDTAG_UNKNOWN;
						break;
				}
			}
			break;

		default:
			elog(WARNING, "unrecognized node type: %d",
				 (int) nodeTag(parsetree));
			tag = CMDTAG_UNKNOWN;
			break;
	}

	return tag;
}
