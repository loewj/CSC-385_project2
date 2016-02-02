#include "cvec.h"
#include "matrix4.h"
#include "visobj.h"

// VisObj constructor
VisObj::VisObj(Matrix4 transform, Cvec3f color, Matrix4* parent) {
  this -> transform = transform;
  this -> identity = transform;
  this -> color = color;
  this -> parent = parent;
}

Matrix4 VisObj::getIdentity() {
  return identity;
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

Matrix4 VisObj::getParent() {
  return *parent;
}

Matrix4 VisObj::getTransform() {
  return transform;
}
