#ifndef PARTICLE_H
#define PARTICLE_H

/** Define colors*/
#define BLUE 1
#define RED 2
#define GREEN 3

/** Define speed factor for particles of different colors
 * Blue is the slowest and GREEN is the fastest
 */
#define SPEED_FACTOR 0.5
//#define RED_SPEED_FACTOR 1.0
//#define GREEN_SPEED_FACTOR 1.5


/*
*Particle object with x&y coordinate components
*/

class Particle  {
public:
    double x;
    double y;
    int color;
    Particle(){};
    Particle(double a, double b, int _color){ x=a; y=b; color = _color; }
    Particle(double a, double b){ x=a; y=b; }
    void pup(PUP::er &p){
    p|x;
    p|y;
    }

};

#endif
