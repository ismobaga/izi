#include "value.h"
#include "chunk.h"

#include <iterator>
#include <string>


ObjFunction::ObjFunction(){
  arity = 0;
  name= "";
  chunk = new Chunk();

}
ObjFunction::~ObjFunction(){
  delete chunk;
}



inline std::ostream& operator<<(std::ostream& os, const Value& v) {
    std::visit(OutputVisitor(), v.as);
    return os;
}
void printValue(Value value) {
     switch (value.type) {
    case VAL_BOOL:
      printf(AS_BOOL(value) ? "true" : "false");
      break;
    case VAL_NIL: printf("nil"); break;
    case VAL_NUMBER: printf("%g", AS_NUMBER(value)); break;
    case VAL_STRING: printf("%s", AS_CSTRING(value)); break;
    case VAL_FUNCTION: 
      if (AS_FUNCTION(value)->name == "") {
        printf("<script>");
        return;
      }
      printf("<fn %s>", AS_FUNCTION(value)->name.c_str()); break;
  }
}
struct FalsinessVisitor {
    bool operator()(const bool b) const { return !b; }
    bool operator()(const std::monostate n) const { return true; }
    
    template <typename T>
    bool operator()(const T& value) const { return false; }
};


bool isFalsey(Value value) {
  return std::visit(FalsinessVisitor(), value.as);
  // return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

bool valuesEqual(Value a, Value b) {
  return a.as == b.as;

  if (a.type != b.type) return false;
  switch (a.type) {
    case VAL_BOOL:   return AS_BOOL(a) == AS_BOOL(b);
    case VAL_NIL:    return true;
    case VAL_NUMBER: return AS_NUMBER(a) == AS_NUMBER(b);
    default:         return false; // Unreachable.
  }
}

std::string copyString(const char* chars, int length){
  return std::string(chars, length);
}