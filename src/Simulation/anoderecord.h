#ifndef ANODERECORD_H
#define ANODERECORD_H

// The chain of linked nodes represents nodes of the same event

class ANodeRecord
{
public:
    static ANodeRecord* createS(double x, double y, double z, double time = 0, int numPhot = -1, ANodeRecord * rec= nullptr);
    static ANodeRecord* createV(double * r, double time = 0, int numPhot = -1, ANodeRecord * rec= nullptr);

    void addLinkedNode(ANodeRecord * node); // ownership is transferred to the chain of linked node of this node

    int    getNumberOfLinkedNodes();
    double getX() const {return R[0];}
    double getY() const {return R[1];}
    double getZ() const {return R[2];}

    ~ANodeRecord(); //deletes linked node

    // prevent creation on the stack and copy/move
private:
    ANodeRecord(double x, double y, double z, double time = 0, int numPhot = -1, ANodeRecord * rec = nullptr);
    ANodeRecord(double * r, double time = 0, int numPhot = -1, ANodeRecord * rec = nullptr);

    ANodeRecord(const ANodeRecord &) = delete;
    ANodeRecord & operator=(const ANodeRecord &) = delete;
    ANodeRecord(ANodeRecord &&) = delete;
    ANodeRecord & operator=(ANodeRecord &&) = delete;

public:
    //double X;
    //double Y;
    //double Z;
    double R[3];
    double Time = 0;
    int    NumPhot = -1; // -1 means no override

    ANodeRecord * getLinkedNode() const {return LinkedNode;}

private:
    ANodeRecord * LinkedNode = nullptr;

};
#endif // ANODERECORD_H
