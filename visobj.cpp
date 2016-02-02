#include "cvec.h"
#include "matrix4.h"
#include "visobj.h"

// VisObj constructor
VisObj::VisObj(Matrix4 transform, Cvec3f color, VisObj* parent) {
  this -> transform = transform;
  this -> color = color;
  this -> parent = parent;
}

void VisObj::setTransform(Matrix4 offset) {
  transform = transform * offset;
}

Cvec3f VisObj::getColor() {
  return color;
}

void VisObj::setColor(Cvec3f newColor) {
  color = newColor;
}

void VisObj::setParent(VisObj* newParent) {
  parent = newParent;
}

Matrix4 VisObj::getTransform() {
  if (parent == NULL){
    return transform;
  } else {
    return Matrix4(parent -> getTransform() * transform);
  }
}
