#ifndef RID_OWNER_H
#define RID_OWNER_H

#include <cstdint>
#include <optional>
#include <unordered_map>

template <typename T> class RIDOwner {
private:
	std::unordered_map<uint64_t, T> _map;
	uint64_t _last = 0;

public:
	T &operator[](uint64_t id) {
		return _map[id];
	}

	std::unordered_map<uint64_t, T> &map() {
		return _map;
	}

	uint64_t insert(T value) {
		_last++;
		_map[_last] = value;

		return _last;
	}

	bool has(uint64_t id) const {
		auto iter = _map.find(id);
		return iter != _map.end();
	}

	T get_id_or_else(uint64_t id, T value) {
		if (has(id))
			return _map[id];

		return value;
	}

	uint64_t size() const {
		return _map.size();
	}

	// returned value needs to be cleaned
	std::optional<T> remove(uint64_t id) {
		auto iter = _map.find(id);

		std::optional<T> value;
		if (iter != _map.end()) {
			value = iter->second;
			_map.erase(iter);
		};

		return value;
	};
};

#endif // !RID_OWNER_H
