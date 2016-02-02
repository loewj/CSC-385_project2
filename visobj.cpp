#include "cvec.h"
#include "matrix4.h"
#include "visobj.h"

// VisObj constructor
VisObj::VisObj(Matrix4 transform, Cvec3f color, Matrix4* parent) {
  this -> transform = transform;
  this -> color = color;
  this -> parent = parent;
}

Cvec3f VisObj::getColor() {
  return color;
}

Matrix4 VisObj::getParent() {
  return *parent;
}

Matrix4 VisObj::getTransform() {
  return transform;
}
