#ifndef OBJECT_OWNER_H
#define OBJECT_OWNER_H

#include <cstdint>
#include <unordered_map>

typedef uint64_t ObjectID;

template <typename T> class ObjectOwner {
private:
	std::unordered_map<ObjectID, T> _map;

public:
	T &operator[](ObjectID id) {
		return _map[id];
	}

	const std::unordered_map<ObjectID, T> &map() {
		return _map;
	}

	ObjectID append(T value) {
		static ObjectID last = 0;
		last++;

		_map[last] = value;
		return last;
	}

	bool has(ObjectID id) const {
		return _map.find(id) != _map.end();
	}

	size_t size() const {
		return _map.size();
	}

	T remove(ObjectID id) {
		if (_map.find(id) == _map.end())
			return {};

		T value = _map[id];
		_map.erase(id);
		return value;
	}
};

#endif // !OBJECT_OWNER_H
