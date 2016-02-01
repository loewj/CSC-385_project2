#ifndef VISOBJ_H
#define VISOBJ_H

class VisObj {

  // Instance variables private by default
  private:
    Matrix4 transform;
    Cvec3f color;
    Matrix4* parent;

  public:
    void setColor(Cvec3f new_color);
    Cvec3f getColor();
    Matrix4* getParent();

};

#endif
