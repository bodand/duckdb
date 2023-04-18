#include "duckdb/common/types/column/column_data_collection.hpp"
#include "duckdb/execution/operator/scan/physical_column_data_scan.hpp"
#include "duckdb/execution/operator/set/physical_recursive_cte.hpp"
#include "duckdb/execution/physical_plan_generator.hpp"
#include "duckdb/planner/expression/bound_reference_expression.hpp"
#include "duckdb/planner/operator/logical_cteref.hpp"
#include "duckdb/planner/operator/logical_recursive_cte.hpp"

namespace duckdb {

unique_ptr<PhysicalOperator> PhysicalPlanGenerator::CreatePlan(LogicalRecursiveCTE &op) {
	D_ASSERT(op.children.size() == 2);

	// Create the working_table that the PhysicalRecursiveCTE will use for evaluation.
	auto working_table = std::make_shared<ColumnDataCollection>(context, op.types);

	// Add the ColumnDataCollection to the context of this PhysicalPlanGenerator
	recursive_cte_tables[op.table_index] = working_table;

	auto left = CreatePlan(*op.children[0]);
	auto right = CreatePlan(*op.children[1]);

	auto cte = make_uniq<PhysicalRecursiveCTE>(op.ctename, op.table_index, op.types, op.union_all, std::move(left),
	                                           std::move(right), op.estimated_cardinality);
	cte->working_table = working_table;

	return std::move(cte);
}

unique_ptr<PhysicalOperator> PhysicalPlanGenerator::CreatePlan(LogicalCTERef &op) {
	D_ASSERT(op.children.empty());

	// Check if this LogicalCTERef is supposed to scan a materialized CTE.
	for (idx_t i = 0; i < materialized_cte_ids.size(); i++) {
		if (op.cte_index == materialized_cte_ids[i]) {
			auto chunk_scan = make_uniq<PhysicalColumnDataScan>(op.types, PhysicalOperatorType::CTE_SCAN,
			                                                    op.estimated_cardinality, op.cte_index);

			auto materialized_cte = materialized_ctes.find(op.cte_index);

			if (materialized_cte == materialized_ctes.end()) {
				throw InvalidInputException("Referenced CTE does not exist.");
			}
			materialized_cte->second.push_back(chunk_scan.get());

			auto cte = recursive_cte_tables.find(op.cte_index);
			if (cte == recursive_cte_tables.end()) {
				throw InvalidInputException("Referenced CTE does not exist.");
			}
			chunk_scan->collection = cte->second.get();

			return std::move(chunk_scan);
		}
	}

	auto chunk_scan = make_uniq<PhysicalColumnDataScan>(op.types, PhysicalOperatorType::RECURSIVE_CTE_SCAN,
	                                                    op.estimated_cardinality, op.cte_index);

	// CreatePlan of a LogicalRecursiveCTE must have happened before.
	auto cte = recursive_cte_tables.find(op.cte_index);
	if (cte == recursive_cte_tables.end()) {
		throw InvalidInputException("Referenced recursive CTE does not exist.");
	}
	chunk_scan->collection = cte->second.get();
	return std::move(chunk_scan);
}

} // namespace duckdb
