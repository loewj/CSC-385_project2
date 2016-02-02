#ifndef VISOBJ_H
#define VISOBJ_H

class VisObj {

  // Instance variables private by default
  private:
    Matrix4 transform;
    Matrix4 identity;
    Cvec3f color;
    Matrix4* parent;

  public:
    VisObj(Matrix4 transform, Cvec3f color, Matrix4* parent);
    Cvec3f getColor();
    void setColor(Cvec3f newColor);
    Matrix4 getParent();
    void setTransform(Matrix4 offset);
    Matrix4 getTransform();
    Matrix4 getIdentity();
};

#endif
