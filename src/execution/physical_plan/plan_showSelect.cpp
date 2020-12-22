#include "duckdb/execution/operator/scan/physical_chunk_scan.hpp"
#include "duckdb/execution/physical_plan_generator.hpp"
#include "duckdb/parser/parsed_data/show_select_info.hpp"
#include "duckdb/planner/operator/logical_show.hpp"

using namespace duckdb;
using namespace std;

unique_ptr<PhysicalOperator> PhysicalPlanGenerator::CreatePlan(LogicalShow &op) {

  DataChunk output;
  output.Initialize(op.types);


  auto collection = make_unique<ChunkCollection>();
	for (idx_t i = 0; i < op.types_select.size(); i++) {
		auto type = op.types_select[i];
		auto &name = op.aliases[i];

		// "name", TypeId::VARCHAR
		output.SetValue(0, output.size(), Value(name));
		// "type", TypeId::VARCHAR
		output.SetValue(1, output.size(), Value(type.ToString()));
		// "notnull", TypeId::BOOL
		output.SetValue(2, output.size(), Value::BOOLEAN(false));
		// "dflt_value", TypeId::VARCHAR
		output.SetValue(3, output.size(), Value());
		// "pk", TypeId::BOOL
		output.SetValue(4, output.size(), Value::BOOLEAN(false));
    output.SetCardinality(output.size() + 1);
		if (output.size() == STANDARD_VECTOR_SIZE) {
			collection->Append(output);
			output.Reset();
		}
	}



  collection->Append(output);

	// create a chunk scan to output the result
	auto chunk_scan = make_unique<PhysicalChunkScan>(op.types, PhysicalOperatorType::CHUNK_SCAN);
	chunk_scan->owned_collection = move(collection);
	chunk_scan->collection = chunk_scan->owned_collection.get();
	return move(chunk_scan);
}
