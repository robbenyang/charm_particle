#include <stdlib.h>
#include <vector>
#include "liveViz.h"
#include "pup_stl.h"
#include "Particle.h"
#include "ParticleExercise.decl.h"

#define RANGE (1.0)
#define ITERATION (500)

#define CELL_WIDTH 100

/*readonly*/ CProxy_Main mainProxy;
/*readonly*/ CProxy_Cell cellProxy;
/*readonly*/ int elementsPerCell;
/*readonly*/ int cellDimension;

CkArrayID id;

using namespace std;

class Main: public CBase_Main {

		public:
				int curr_max;
				int counter;
				int doneCells;
				Main(CkArgMsg* m) {
						curr_max = -1;
						counter = 0;
						doneCells = 0;
						if(m->argc < 3) CkAbort("SUcmiAtGE: ./charmrun +p<number_of_processors> ./particle <number of particles per cell> <size of array>");

						mainProxy = thisProxy;
						elementsPerCell = atoi(m->argv[1]);
						cellDimension = atoi(m->argv[2]);
						delete m;

						//create the grid and start the simulation by calling run()
						cellProxy = CProxy_Cell::ckNew(cellDimension, cellDimension);
						//CkArrayOptions opts(cellDimension, cellDimension);
						//cellProxy = CProxy_Type::ckNew(opts);
						
						
						//setup liveViz
						CkCallback liveViz_cb(CkIndex_Cell::requestNextFrame(0), cellProxy);//Not sure how it works
						liveVizConfig cfg(liveVizConfig::pix_color, true);
						liveVizInit(cfg, id, liveViz_cb);

						//Create a reduction callback
						CkCallback * reduction_cb = new CkCallback(CkReductionTarget(Main, myReduction), mainProxy);
						cellProxy.ckSetReductionClient(reduction_cb);

						cellProxy.run();
				}

				// TODO: add an entry method which will be a target of the reduction for counts
				void myReduction(int max_num){
					//curr_max = curr_max > max_num ? curr_max : max_num;
					CkPrintf("Counter = %d\n", counter++);
					CkPrintf("max number in this iteration = %d\n", max_num);

				}


				//after every chare is done, main chare exits by calling CkExit()
				void done() {
						doneCells++;
						if(doneCells >= cellDimension*cellDimension){
								CkPrintf("EXIT %d\n", doneCells);
								CkExit();
						}
				}
};

// This class represent the cells of the simulation.
/// Each cell contains a vector of particle.
// On each time step, the cell perturbs the particles and moves them to neighboring cells as necessary.
class Cell: public CBase_Cell {
		Cell_SDAG_CODE

		public:
				double xMin, xMax, yMin, yMax;
				int iteration;

				// Temporary variables for use within Structured Dagger
				int remoteNeighbors;

				vector<Particle> particles;

				// Collects the particles that need transferring to neighborring cells
				vector<Particle> N, E, W, S, O;

				Cell() {
						__sdag_init();
						iteration = 1;
						initializeBounds();
						populateCell(elementsPerCell); //creates random particles within the cell
						usesAtSync = true;

				}

				Cell(CkMigrateMessage* m) {}

				void pup(PUP::er &p){
						CBase_Cell::pup(p);
						__sdag_pup(p);
						p|xMin; p|yMin; p|xMax; p|yMax;
						p|iteration;
						p|particles;
						p|N; p|E; p|O; p|W; p|S;
				}

				void updateParticles() {
						clearPreviouslyCollectedParticles();
						for(int index = 0; index < particles.size(); index++) {
								perturb(&particles[index]);
								moveParticleAtIndex(&particles[index]);
						}
						//update particles remaining current cell
						particles = O; //This is varibale "O" not 0.

				}

				void requestNextFrame(liveVizRequestMsg * m){
					double cell_width = RANGE/cellDimension;

					int block_width = CELL_WIDTH;
					int sx = thisIndex.x * block_width;
					int sy = thisIndex.y * block_width;


					int len = 3*block_width*block_width;
					unsigned char * intensity = new unsigned char[len];

					for(int i = 0; i < len; i++){
						intensity[i] = 255;
					}


					//Draw particles on the canvas
					for(int i = 0; i < particles.size(); i++){
						//CkPrintf("Floating point:%f, %f, %d\n", particles[i].x, particles[i].y, particles[i].color);
						int currX = (int)((particles[i].x - xMin)*block_width/cell_width);
						int currY = (int)((particles[i].y - yMin)*block_width/cell_width);
						int color = particles[i].color;
						int coord = 3*(currY * block_width + currX);
						if(color == RED){
							//Not colored by other colors yet, don't want to color 2 pixel twice, or it will become black.
							if(intensity[coord] != 0){
								intensity[coord + 1] = 0;
								intensity[coord + 2] = 0;
							}
						}
						else if(color == GREEN){
							if(intensity[coord + 1] != 0){
								intensity[coord + 0] = 0;
								intensity[coord + 2] = 0;	
							}
						}
						else{
							if(intensity[coord + 2] != 0){
								intensity[coord + 0] = 0;
								intensity[coord + 1] = 0;
							}
						}
					}
					
					int addConst = 3 * block_width; //Define this to avoid unnecessary calculation
					//Draw verticle lines
					if(thisIndex.x != 0){
						for(int i = addConst; i < len; i += addConst) {
							intensity[i] = 0;
							intensity[i+1] = 0;
							intensity[i+2] = 0;
						}
					}

					//Draw horizontal lines
					if(thisIndex.y + 1 != cellDimension){
						for(int i = 3 * block_width * (block_width - 1); i < len; i++){
							intensity[i] = 0;
							intensity[i+1] = 0;
							intensity[i+2] = 0;
						}
					}

					liveVizDeposit(m, sx, sy, block_width, block_width, intensity, this);
					delete [] intensity;

				}


		private:

				void initializeBounds() {
						xMin = RANGE * thisIndex.x / cellDimension;
						xMax = RANGE * (thisIndex.x + 1) / cellDimension;
						yMin = RANGE * thisIndex.y / cellDimension;
						yMax = RANGE * (thisIndex.y + 1 ) / cellDimension;
				}

				void insertRedElems(int initialElements){
					for(int element = 0; element < initialElements; element++) {
								Particle p = Particle(randomWithin(xMin, xMax), randomWithin(yMin, yMax), RED);
								particles.push_back(p);
						}
				}
				
				void populateCell(int initialElements) {
					int oneFourth = cellDimension / 4;
					int threeFourth = 3 * cellDimension / 4;
					if(thisIndex.x < cellDimension/2){
						for(int element = 0; element < initialElements; element++) {
								Particle p = Particle(randomWithin(xMin, xMax), randomWithin(yMin, yMax), GREEN);
								particles.push_back(p);
						}
						if(thisIndex.x < oneFourth && (thisIndex.y < oneFourth || thisIndex.y >= threeFourth))
							insertRedElems(initialElements);
					}
					else{
						for(int element = 0; element < initialElements; element++) {
								Particle p = Particle(randomWithin(xMin, xMax), randomWithin(yMin, yMax), BLUE);
								particles.push_back(p);
						}
						if(thisIndex.x >= threeFourth && (thisIndex.y < oneFourth || thisIndex.y >= threeFourth))
							insertRedElems(initialElements);
					}
				}

				double randomWithin(double min, double max) {
						double random = drand48();
						return min + random * (max - min);
				}

				void clearPreviouslyCollectedParticles() {
						N.clear(); E.clear(); O.clear(); W.clear(); S.clear();;
				}

				void perturb(Particle* particle) {

						float maxDelta = 0.01; //a const that determines the speed of the particle movement
						double deltax = drand48()*maxDelta - maxDelta/2.0;
						double deltay = drand48()*maxDelta - maxDelta/2.0;

						if(particle->color == BLUE || particle->color == GREEN){
							deltax *= SPEED_FACTOR;
							deltay *= SPEED_FACTOR;
						}
						
						//particle moves into either x or into y direction randomly.
						double direction = drand48();
						if(direction >= 0.5 )
								particle->x += deltax;
						else particle->y += deltay;
				}


				// This is kind of tricky. You need to decide where to send the particle
				// based on its +deltax/+deltay values but after that you need to remember
				// to reset the values so that it does not exceed the bounds
				void moveParticleAtIndex(Particle* particle) {
						double x = particle->x;
						double y = particle->y;

						double newX = x;
						double newY = y;
						if(newX > RANGE) newX -= 1.0;
						else if(newX < 0.0) newX += 1.0;

						if(newY > RANGE) newY -= 1.0;
						else if(newY < 0.0) newY += 1.0;
						Particle temp(newX, newY, particle->color);

						// put the particle into the correct vector to be sended
						if(y > yMax) {
								N.push_back(temp);
						} else if (x < xMin) {
								E.push_back(temp);
						} else if (x > xMax) {
								W.push_back(temp);
						} else if (y < yMin) {
								S.push_back(temp);
						} else {
								O.push_back(temp);
						}

				}

				int wrap(int w) {
						if (w >= cellDimension) return 0;
						else if (w < 0) return cellDimension - 1;
						else return w;
				}

};

#include "ParticleExercise.def.h"
