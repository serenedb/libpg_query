// clang-format off
/*-------------------------------------------------------------------------
 *
 * parse_target.c
 *	  handle target lists
 *
 * Portions Copyright (c) 1996-2025, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  src/backend/parser/parse_target.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "parser/parse_target.h"

static int	FigureColnameInternal(Node *node, char **name);

/*
 * FigureColname -
 *	  if the name of the resulting column is not specified in the target
 *	  list, we have to guess a suitable name.  The SQL spec provides some
 *	  guidance, but not much...
 *
 * Note that the argument is the *untransformed* parse tree for the target
 * item.  This is a shade easier to work with than the transformed tree.
 */
char *
FigureColname(Node *node)
{
	char	   *name = NULL;

	(void) FigureColnameInternal(node, &name);
	if (name != NULL)
		return name;
	/* default result if we can't guess anything */
	return "?column?";
}

/*
 * FigureIndexColname -
 *	  choose the name for an expression column in an index
 *
 * This is actually just like FigureColname, except we return NULL if
 * we can't pick a good name.
 */
char *
FigureIndexColname(Node *node)
{
	char	   *name = NULL;

	(void) FigureColnameInternal(node, &name);
	return name;
}

/*
 * FigureColnameInternal -
 *	  internal workhorse for FigureColname
 *
 * Return value indicates strength of confidence in result:
 *		0 - no information
 *		1 - second-best name choice
 *		2 - good name choice
 * The return value is actually only used internally.
 * If the result isn't zero, *name is set to the chosen name.
 */
static int
FigureColnameInternal(Node *node, char **name)
{
	int			strength = 0;

	if (node == NULL)
		return strength;

	switch (nodeTag(node))
	{
		case T_ColumnRef:
			{
				char	   *fname = NULL;
				ListCell   *l;

				/* find last field name, if any, ignoring "*" */
				foreach(l, ((ColumnRef *) node)->fields)
				{
					Node	   *i = lfirst(l);

					if (IsA(i, String))
						fname = strVal(i);
				}
				if (fname)
				{
					*name = fname;
					return 2;
				}
			}
			break;
		case T_A_Indirection:
			{
				A_Indirection *ind = (A_Indirection *) node;
				char	   *fname = NULL;
				ListCell   *l;

				/* find last field name, if any, ignoring "*" and subscripts */
				foreach(l, ind->indirection)
				{
					Node	   *i = lfirst(l);

					if (IsA(i, String))
						fname = strVal(i);
				}
				if (fname)
				{
					*name = fname;
					return 2;
				}
				return FigureColnameInternal(ind->arg, name);
			}
			break;
		case T_FuncCall:
			*name = strVal(llast(((FuncCall *) node)->funcname));
			return 2;
		case T_A_Expr:
			if (((A_Expr *) node)->kind == AEXPR_NULLIF)
			{
				/* make nullif() act like a regular function */
				*name = "nullif";
				return 2;
			}
			break;
		case T_TypeCast:
			strength = FigureColnameInternal(((TypeCast *) node)->arg,
											 name);
			if (strength <= 1)
			{
				if (((TypeCast *) node)->typeName != NULL)
				{
					*name = strVal(llast(((TypeCast *) node)->typeName->names));
					return 1;
				}
			}
			break;
		case T_CollateClause:
			return FigureColnameInternal(((CollateClause *) node)->arg, name);
		case T_GroupingFunc:
			/* make GROUPING() act like a regular function */
			*name = "grouping";
			return 2;
		case T_MergeSupportFunc:
			/* make MERGE_ACTION() act like a regular function */
			*name = "merge_action";
			return 2;
		case T_SubLink:
			switch (((SubLink *) node)->subLinkType)
			{
				case EXISTS_SUBLINK:
					*name = "exists";
					return 2;
				case ARRAY_SUBLINK:
					*name = "array";
					return 2;
				case EXPR_SUBLINK:
					{
						/* Get column name of the subquery's single target */
						SubLink    *sublink = (SubLink *) node;
						Query	   *query = (Query *) sublink->subselect;

						/*
						 * The subquery has probably already been transformed,
						 * but let's be careful and check that.  (The reason
						 * we can see a transformed subquery here is that
						 * transformSubLink is lazy and modifies the SubLink
						 * node in-place.)
						 */
						if (IsA(query, Query))
						{
							TargetEntry *te = (TargetEntry *) linitial(query->targetList);

							if (te->resname)
							{
								*name = te->resname;
								return 2;
							}
						}
					}
					break;
					/* As with other operator-like nodes, these have no names */
				case MULTIEXPR_SUBLINK:
				case ALL_SUBLINK:
				case ANY_SUBLINK:
				case ROWCOMPARE_SUBLINK:
				case CTE_SUBLINK:
					break;
			}
			break;
		case T_CaseExpr:
			strength = FigureColnameInternal((Node *) ((CaseExpr *) node)->defresult,
											 name);
			if (strength <= 1)
			{
				*name = "case";
				return 1;
			}
			break;
		case T_A_ArrayExpr:
			/* make ARRAY[] act like a function */
			*name = "array";
			return 2;
		case T_RowExpr:
			/* make ROW() act like a function */
			*name = "row";
			return 2;
		case T_CoalesceExpr:
			/* make coalesce() act like a regular function */
			*name = "coalesce";
			return 2;
		case T_MinMaxExpr:
			/* make greatest/least act like a regular function */
			switch (((MinMaxExpr *) node)->op)
			{
				case IS_GREATEST:
					*name = "greatest";
					return 2;
				case IS_LEAST:
					*name = "least";
					return 2;
			}
			break;
		case T_SQLValueFunction:
			/* make these act like a function or variable */
			switch (((SQLValueFunction *) node)->op)
			{
				case SVFOP_CURRENT_DATE:
					*name = "current_date";
					return 2;
				case SVFOP_CURRENT_TIME:
				case SVFOP_CURRENT_TIME_N:
					*name = "current_time";
					return 2;
				case SVFOP_CURRENT_TIMESTAMP:
				case SVFOP_CURRENT_TIMESTAMP_N:
					*name = "current_timestamp";
					return 2;
				case SVFOP_LOCALTIME:
				case SVFOP_LOCALTIME_N:
					*name = "localtime";
					return 2;
				case SVFOP_LOCALTIMESTAMP:
				case SVFOP_LOCALTIMESTAMP_N:
					*name = "localtimestamp";
					return 2;
				case SVFOP_CURRENT_ROLE:
					*name = "current_role";
					return 2;
				case SVFOP_CURRENT_USER:
					*name = "current_user";
					return 2;
				case SVFOP_USER:
					*name = "user";
					return 2;
				case SVFOP_SESSION_USER:
					*name = "session_user";
					return 2;
				case SVFOP_CURRENT_CATALOG:
					*name = "current_catalog";
					return 2;
				case SVFOP_CURRENT_SCHEMA:
					*name = "current_schema";
					return 2;
			}
			break;
		case T_XmlExpr:
			/* make SQL/XML functions act like a regular function */
			switch (((XmlExpr *) node)->op)
			{
				case IS_XMLCONCAT:
					*name = "xmlconcat";
					return 2;
				case IS_XMLELEMENT:
					*name = "xmlelement";
					return 2;
				case IS_XMLFOREST:
					*name = "xmlforest";
					return 2;
				case IS_XMLPARSE:
					*name = "xmlparse";
					return 2;
				case IS_XMLPI:
					*name = "xmlpi";
					return 2;
				case IS_XMLROOT:
					*name = "xmlroot";
					return 2;
				case IS_XMLSERIALIZE:
					*name = "xmlserialize";
					return 2;
				case IS_DOCUMENT:
					/* nothing */
					break;
			}
			break;
		case T_XmlSerialize:
			/* make XMLSERIALIZE act like a regular function */
			*name = "xmlserialize";
			return 2;
		case T_JsonParseExpr:
			/* make JSON act like a regular function */
			*name = "json";
			return 2;
		case T_JsonScalarExpr:
			/* make JSON_SCALAR act like a regular function */
			*name = "json_scalar";
			return 2;
		case T_JsonSerializeExpr:
			/* make JSON_SERIALIZE act like a regular function */
			*name = "json_serialize";
			return 2;
		case T_JsonObjectConstructor:
			/* make JSON_OBJECT act like a regular function */
			*name = "json_object";
			return 2;
		case T_JsonArrayConstructor:
		case T_JsonArrayQueryConstructor:
			/* make JSON_ARRAY act like a regular function */
			*name = "json_array";
			return 2;
		case T_JsonObjectAgg:
			/* make JSON_OBJECTAGG act like a regular function */
			*name = "json_objectagg";
			return 2;
		case T_JsonArrayAgg:
			/* make JSON_ARRAYAGG act like a regular function */
			*name = "json_arrayagg";
			return 2;
		case T_JsonFuncExpr:
			/* make SQL/JSON functions act like a regular function */
			switch (((JsonFuncExpr *) node)->op)
			{
				case JSON_EXISTS_OP:
					*name = "json_exists";
					return 2;
				case JSON_QUERY_OP:
					*name = "json_query";
					return 2;
				case JSON_VALUE_OP:
					*name = "json_value";
					return 2;
					/* JSON_TABLE_OP can't happen here. */
				default:
					elog(ERROR, "unrecognized JsonExpr op: %d",
						 (int) ((JsonFuncExpr *) node)->op);
			}
			break;
		default:
			break;
	}

	return strength;
}
