#ifndef VISOBJ_H
#define VISOBJ_H

class VisObj {

  // Instance variables private by default
  private:
    Matrix4 transform;
    Cvec3f color;
    VisObj* parent;

  public:
    VisObj(Matrix4 transform, Cvec3f color, VisObj* parent);
    Cvec3f getColor();
    void setColor(Cvec3f newColor);
    VisObj* getParent();
    void setParent(VisObj* newParent);
    void setTransform(Matrix4 offset);
    Matrix4 getTransform();
};

#endif
