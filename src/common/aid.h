#ifndef ANTS2_AID_H
#define ANTS2_AID_H

///
/// \brief An ID template used for type-safety
///
template<typename T, typename TID = unsigned int>
struct AId {
  typedef AId<T, TID> type;
  typedef T handled_type;
  typedef TID value_type;

private:
  value_type id;

public:
  AId() {}
  AId(const type &other) : id(other.id) {}
  explicit AId(const value_type &id) : id(id) {}
  AId<T, value_type> &operator=(const type &other) { id = other.id; return *this; }

  value_type val() const { return id; }
  //explicit operator value_type() const { return id; }

  bool operator==(const type &other) const { return id == other.id; }
  bool operator!=(const type &other) const { return id != other.id; }
  bool operator<(const type &other) const { return id < other.id; }

  //If you get an error here then it is because you called a function which is
  //not present in the underlying value_type of an instantiated class
  type &operator++() { ++id; return *this; }
  type operator++(int /*postfix*/) { return type(id++); }

  type operator+(const type &other) const { return type(id+other.id); }
  type operator-(const type &other) const { return type(id-other.id); }
};

#endif // ANTS2_AID_H
