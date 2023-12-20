//===----------------------------------------------------------------------===//
//                         DuckDB
//
// duckdb/execution/operator/scan/csv/scanner/base_scanner.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "duckdb/execution/operator/scan/csv/csv_buffer_manager.hpp"
#include "duckdb/execution/operator/scan/csv/csv_state_machine.hpp"
#include "duckdb/execution/operator/scan/csv/parser/scanner_boundary.hpp"

namespace duckdb {

class StringValueResult : public ScannerResult {
public:
	unique_ptr<Vector> vector;
	string_t *vector_ptr;
	idx_t vector_size;

	idx_t last_position;
	idx_t cur_value_idx;

	char *buffer_ptr;

	//! Adds a Value to the result
	static inline void AddValue(StringValueResult &result, const char current_char, const idx_t buffer_pos);
	//! Adds a Row to the result
	static inline bool AddRow(StringValueResult &result, const char current_char, const idx_t buffer_pos);
	//! Behavior when hitting an invalid state
	static inline void Kaput(StringValueResult &result);

	//! Returns a DataChunk
};

//! Our dialect scanner basically goes over the CSV and actually parses the values to a DuckDB vector of string_t
class StringValueScanner : public BaseScanner {
public:
	StringValueScanner(shared_ptr<CSVBufferManager> buffer_manager, shared_ptr<CSVStateMachine> state_machine);

	StringValueResult *ParseChunk() override;

private:
	void Process() override;

	void FinalizeChunkProcess() override;

	//! Function used to process values that go over the first buffer, extra allocation might be necessary
	void ProcessOverbufferValue();

	//! Function used to move from one buffer to the other, if necessary
	void MoveToNextBuffer();

	StringValueResult result;

	//! Pointer to the previous buffer handle, necessary for overbuffer values
	unique_ptr<CSVBufferHandle> previous_buffer_handle;
};

} // namespace duckdb
