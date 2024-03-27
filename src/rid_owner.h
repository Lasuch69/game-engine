#ifndef RID_OWNER_H
#define RID_OWNER_H

#include <cstdint>
#include <optional>
#include <unordered_map>

template <typename Value> class RIDOwner {
private:
	std::unordered_map<uint64_t, Value> _map;
	uint64_t _last = 0;

public:
	Value &operator[](uint64_t id) { return _map[id]; }

	std::unordered_map<uint64_t, Value> &map() { return _map; }

	uint64_t insert(Value value) {
		_last++;
		_map[_last] = value;

		return _last;
	}

	bool has(uint64_t id) const {
		auto iter = _map.find(id);
		return iter != _map.end();
	}

	uint64_t count() const {
		uint64_t count = 0;
		for (const auto &[id, _] : _map) {
			count++;
		}

		return count;
	}

	// returned value needs to be cleaned
	std::optional<Value> remove(uint64_t id) {
		auto iter = _map.find(id);

		std::optional<Value> value;
		if (iter != _map.end()) {
			value = iter->second;
			_map.erase(iter);
		};

		return value;
	};
};

#endif // !RID_OWNER_H
