//===----------------------------------------------------------------------===//
//                         DuckDB
//
// duckdb/function/cast/default_casts.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "duckdb/common/types.hpp"
#include "duckdb/common/types/vector.hpp"

namespace duckdb {

class CastFunctionSet;
struct FunctionLocalState;

//! Extra data that can be attached to a bind function of a cast, and is available during binding
struct BindCastInfo {
	DUCKDB_API virtual ~BindCastInfo();
};

//! Extra data that can be returned by the bind of a cast, and is available during execution of a cast
struct BoundCastData {
	DUCKDB_API virtual ~BoundCastData();

	DUCKDB_API virtual unique_ptr<BoundCastData> Copy() const = 0;
};

struct CastParameters {
	CastParameters() {
	}
	CastParameters(BoundCastData *cast_data, bool strict, string *error_message, FunctionLocalState *local_state)
	    : cast_data(cast_data), strict(strict), error_message(error_message), local_state(local_state) {
	}
	CastParameters(CastParameters &parent, BoundCastData *cast_data = nullptr)
	    : cast_data(cast_data), strict(parent.strict), error_message(parent.error_message) {
	}

	//! The bound cast data (if any)
	BoundCastData *cast_data = nullptr;
	//! whether or not to enable strict casting
	bool strict = false;
	// out: error message in case cast has failed
	string *error_message = nullptr;
	//! Local state
	FunctionLocalState *local_state = nullptr;
};

typedef bool (*cast_function_t)(Vector &source, Vector &result, idx_t count, CastParameters &parameters);
typedef unique_ptr<FunctionLocalState> (*init_cast_local_state_t)(ClientContext &context);

struct BoundCastInfo {
	DUCKDB_API
	BoundCastInfo(
	    cast_function_t function, unique_ptr<BoundCastData> cast_data = nullptr,
	    init_cast_local_state_t init_local_state = nullptr); // NOLINT: allow explicit cast from cast_function_t
	cast_function_t function;
	init_cast_local_state_t init_local_state;
	unique_ptr<BoundCastData> cast_data;

public:
	BoundCastInfo Copy() const;
};

struct BindCastInput {
	DUCKDB_API BindCastInput(CastFunctionSet &function_set, BindCastInfo *info, ClientContext *context);

	CastFunctionSet &function_set;
	BindCastInfo *info;
	ClientContext *context;

public:
	DUCKDB_API BoundCastInfo GetCastFunction(const LogicalType &source, const LogicalType &target);
};

struct ListBoundCastData : public BoundCastData {
	explicit ListBoundCastData(BoundCastInfo child_cast) : child_cast_info(std::move(child_cast)) {
	}

	BoundCastInfo child_cast_info;
	static unique_ptr<BoundCastData> BindListToListCast(BindCastInput &input, const LogicalType &source,
	                                                    const LogicalType &target);

public:
	unique_ptr<BoundCastData> Copy() const override {
		return make_unique<ListBoundCastData>(child_cast_info.Copy());
	}
};

struct ListCast {
	static bool ListToListCast(Vector &source, Vector &result, idx_t count, CastParameters &parameters);
};

struct StructBoundCastData : public BoundCastData {
	StructBoundCastData(vector<BoundCastInfo> child_casts, LogicalType target_p)
	    : child_cast_info(std::move(child_casts)), target(std::move(target_p)) {
	}

	vector<BoundCastInfo> child_cast_info;
	LogicalType target;

	static unique_ptr<BoundCastData> BindStructToStructCast(BindCastInput &input, const LogicalType &source,
	                                                        const LogicalType &target);

public:
	unique_ptr<BoundCastData> Copy() const override {
		vector<BoundCastInfo> copy_info;
		for (auto &info : child_cast_info) {
			copy_info.push_back(info.Copy());
		}
		return make_unique<StructBoundCastData>(std::move(copy_info), target);
	}
};

struct MapBoundCastData : public BoundCastData {
	MapBoundCastData(BoundCastInfo key_cast, BoundCastInfo value_cast)
	    : key_cast(move(key_cast)), value_cast(move(value_cast)) {
	}

	BoundCastInfo key_cast;
	BoundCastInfo value_cast;

	static unique_ptr<BoundCastData> BindMapToMapCast(BindCastInput &input, const LogicalType &source,
	                                                  const LogicalType &target);

public:
	unique_ptr<BoundCastData> Copy() const override {
		return make_unique<MapBoundCastData>(key_cast.Copy(), value_cast.Copy());
	}
};

struct DefaultCasts {
	DUCKDB_API static BoundCastInfo GetDefaultCastFunction(BindCastInput &input, const LogicalType &source,
	                                                       const LogicalType &target);

	DUCKDB_API static bool NopCast(Vector &source, Vector &result, idx_t count, CastParameters &parameters);
	DUCKDB_API static bool TryVectorNullCast(Vector &source, Vector &result, idx_t count, CastParameters &parameters);
	DUCKDB_API static bool ReinterpretCast(Vector &source, Vector &result, idx_t count, CastParameters &parameters);

private:
	static BoundCastInfo BlobCastSwitch(BindCastInput &input, const LogicalType &source, const LogicalType &target);
	static BoundCastInfo BitCastSwitch(BindCastInput &input, const LogicalType &source, const LogicalType &target);
	static BoundCastInfo DateCastSwitch(BindCastInput &input, const LogicalType &source, const LogicalType &target);
	static BoundCastInfo DecimalCastSwitch(BindCastInput &input, const LogicalType &source, const LogicalType &target);
	static BoundCastInfo EnumCastSwitch(BindCastInput &input, const LogicalType &source, const LogicalType &target);
	static BoundCastInfo IntervalCastSwitch(BindCastInput &input, const LogicalType &source, const LogicalType &target);
	static BoundCastInfo ListCastSwitch(BindCastInput &input, const LogicalType &source, const LogicalType &target);
	static BoundCastInfo NumericCastSwitch(BindCastInput &input, const LogicalType &source, const LogicalType &target);
	static BoundCastInfo MapCastSwitch(BindCastInput &input, const LogicalType &source, const LogicalType &target);
	static BoundCastInfo PointerCastSwitch(BindCastInput &input, const LogicalType &source, const LogicalType &target);
	static BoundCastInfo StringCastSwitch(BindCastInput &input, const LogicalType &source, const LogicalType &target);
	static BoundCastInfo StructCastSwitch(BindCastInput &input, const LogicalType &source, const LogicalType &target);
	static BoundCastInfo TimeCastSwitch(BindCastInput &input, const LogicalType &source, const LogicalType &target);
	static BoundCastInfo TimeTzCastSwitch(BindCastInput &input, const LogicalType &source, const LogicalType &target);
	static BoundCastInfo TimestampCastSwitch(BindCastInput &input, const LogicalType &source,
	                                         const LogicalType &target);
	static BoundCastInfo TimestampTzCastSwitch(BindCastInput &input, const LogicalType &source,
	                                           const LogicalType &target);
	static BoundCastInfo TimestampNsCastSwitch(BindCastInput &input, const LogicalType &source,
	                                           const LogicalType &target);
	static BoundCastInfo TimestampMsCastSwitch(BindCastInput &input, const LogicalType &source,
	                                           const LogicalType &target);
	static BoundCastInfo TimestampSecCastSwitch(BindCastInput &input, const LogicalType &source,
	                                            const LogicalType &target);
	static BoundCastInfo UnionCastSwitch(BindCastInput &input, const LogicalType &source, const LogicalType &target);
	static BoundCastInfo UUIDCastSwitch(BindCastInput &input, const LogicalType &source, const LogicalType &target);

	static BoundCastInfo ImplicitToUnionCast(BindCastInput &input, const LogicalType &source,
	                                         const LogicalType &target);
};

} // namespace duckdb
