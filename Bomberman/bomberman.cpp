#pragma GCC optimize "O3,omit-frame-pointer,inline"

#define True true
#define False false

#include <iostream>
#include <string>
#include <vector>
#include <list>
#include <algorithm>
#include <chrono>
#include <unistd.h>
#include <set>
#include <stdlib.h>
#include <climits>
#include <queue>

using namespace std;

const signed char GLOBAL_TURN_TIME = 100;
const signed char GLOBAL_TURN_TIME_MAX = 93;
const int GLOBAL_TURN_TIME_MAX_FIRST_TURN = 450;
const signed char GLOBAL_GENOME_SIZE = 16;
const uint GLOBAL_GENOME_SAMPLE_SIZE = 50000;
const signed char GLOBAL_MAX_WIDTH = 13;
const signed char GLOBAL_MAX_HEIGHT = 11;
bool global_debug = false;
const signed char GLOBAL_PLAYER_NUM = 4;
const float GLOBAL_MUTATION_RATE = 0.15;
const uint GLOBAL_POPULATION_SIZE = 2000;

uint global_compute = 0;

struct Board;
Board* global_board;

struct Timer{
    chrono::time_point<chrono::system_clock> end;
    signed char not_always = 0;

    inline Timer() = default;
    inline Timer(Timer const&) = default;
    inline Timer(Timer&&) = default;
    inline Timer& operator=(Timer const&) = default;
    inline Timer& operator=(Timer&&) = default;

    inline Timer(bool first_turn=false){
        if (first_turn) {
            this->end=chrono::system_clock::now()+chrono::milliseconds(GLOBAL_TURN_TIME_MAX_FIRST_TURN);
        } else {
            this->end=chrono::system_clock::now()+chrono::milliseconds(GLOBAL_TURN_TIME_MAX);
        }
    }
    inline bool isTimesUp(){
        return std::chrono::system_clock::now() > this->end;
    }
};
Timer* global_timer = NULL;

struct Point
{
    inline Point(Point&&) = default;
    inline Point& operator=(Point const&) = default;
    inline Point& operator=(Point&&) = default;
    char x;
    char y;
    inline Point() {
        this->x = -1;
        this->y = -1;
    }
    inline Point(const Point& p) {
        this->x = p.x;
        this->y = p.y;
    }
    inline Point(char x1,char y1) : x(x1), y(y1) {
    }
    inline string toString() const
    {
        return std::to_string(this->x) + " " + std::to_string(this->y);
    }
    inline string toString(string debug) const
    {
        return this->toString() + " " + debug;
    }
    inline void correctBounds() {
        if (this->x < 0) {
            this->x = 0;
        }
        if (this->y < 0) {
            this->y = 0;
        }
        if (this->x >= GLOBAL_MAX_WIDTH) {
            this->x = GLOBAL_MAX_WIDTH;
        }
        if (this->y >= GLOBAL_MAX_HEIGHT) {
            this->y = GLOBAL_MAX_HEIGHT;
        }
    }
    inline bool operator==(const Point& p) const {
        return this->x == p.x && this->y == p.y;
    }
    // inline bool operator==(const Point& p1, const Point& p2) {
    //     return p1.x == p2.x && p1.y == p2.y;
    // }
};

struct Player {
    char id;
    char range;
    char cur_stock;
    char reloading_stock;
    int score;
    Point p;
    bool isAlive;

    inline Player() = default;
    inline Player(Player const&) = default;
    inline Player(Player&&) = default;
    inline Player& operator=(Player const&) = default;
    inline Player& operator=(Player&&) = default;

    inline Player(const char & id, const Point & p) {
        this->id = id;
        this->range = 3;
        this->cur_stock = 1;
        this->reloading_stock = 0;
        this->p = p;
        this->isAlive = true;
    }

    inline void update(const char& owner_id, const Point & p){
        this->id = owner_id;
        this->p = p;
    }
    inline void reload(){
        this->cur_stock += this->reloading_stock;
        this->reloading_stock = 0;
    }
    inline bool hasBomb(){
        return this->cur_stock > 0;
    }

    inline void increaseScore(){
        ++this->score;
    }
    inline void increaseScore(char n) {
        this->score += n;
    }
    inline void increaseScore(int n) {
        this->score += n;
    }
    inline void kill() {
        this->isAlive = false;
    }
    inline bool operator==(const Player& player) const {
        return  this->id == player.id ;
    }
    inline string toString() const
    {
        return " id: " + to_string(this->id) +
               " range: " + to_string(this->range) +
               " cur_stock: " + to_string(this->cur_stock) +
               " score: " + to_string(this->score) +
               " isAlive: " + to_string(this->isAlive) +
               " position " + this->p.toString();
    }
};

struct Bomb {
    char owner;
    char range;
    char timer;
    Point p;
    Bomb* previous_bomb = NULL;
    Bomb* next_bomb = NULL;


    inline Bomb() = default;
    inline Bomb(Bomb const&) = default;
    inline Bomb(Bomb&&) = default;
    inline Bomb& operator=(Bomb const&) = default;
    inline Bomb& operator=(Bomb&&) = default;

    inline Bomb(const char & owner,const char & range,const char & timer, const Point & p) {
        this->owner = owner;
        this->range = range;
        this->timer = timer;
        this->p = p;
    }

    inline void tick(){
        --this->timer;
    }

    inline bool isExploding(){
        return this->timer <= 0;
    }
    inline bool operator==(const Bomb& b) const {
        return  this->owner == b.owner && this->p == b.p;
    }
};

struct Square {
    enum type { empty, player, bomb, bomb_player,box, box_b_range, box_b_stock, item_b_range,item_b_stock, wall};
    type t;
    Point p;

    inline Square(Square const&) = default;
    inline Square(Square&&) = default;
    inline Square& operator=(Square const&) = default;
    inline Square& operator=(Square&&) = default;

    inline Square() {
        this->p = Point();
        this->setEmpty();
    }
    inline Square(const Point& p) {
        this->p = p;
        this->setEmpty();
    }
    inline void addBomb(){
		if(this->t == player){
			this->t = type::bomb_player;
		} else{
			this->t = type::bomb;
		}
    }
    inline void addBox(const char& box_type ){
        if (box_type == '2') {
            this->t=type::box_b_stock;
        } else if (box_type == '1') {
            this->t=type::box_b_range;
        } else {
            this->t=type::box;
        }
    }
    inline void addWall(){
        this->t=type::wall;
    }
    inline void setEmpty(){
        this->t=type::empty;
    }
    inline bool canEnter() const {
        return (this->t == type::empty ||
				this->t == type::item_b_range ||
				this->t == type::item_b_stock);

    }
    inline bool isBox() const {
        return (this->t == type::box ||
				this->t == type::box_b_range ||
				this->t == type::box_b_stock);
    }
    inline bool hasBonus() const {
        return (this->t == type::box_b_range  ||
				this->t == type::box_b_stock  ||
				this->t == type::item_b_range ||
				this->t == type::item_b_stock );
    }
    inline void removeBonus()  {
		if(this->t == type::item_b_range ||
           this->t == type::item_b_stock) {
			   this->t = type::empty;
		}
    }
    inline void addPlayer() {
        if(this->t == type::bomb){
			this->t = type::bomb_player;
		} else{
			this->t = type::player;
		}
    }
    inline void removePlayer() {
        if(this->t == type::bomb_player){
			this->t = type::bomb;
		} else{
			this->t = type::empty;
		}
    }
    inline void addItem(char param1){
        if(param1 == 1){
            if (this->t == type::box){
                this->t = type::box_b_range;
            } else {
                this->t = type::item_b_range;
            }
        } else {
            if (this->t == type::box){
                this->t = type::box_b_stock;
            } else {
                this->t = type::item_b_stock;
            }
        }
    }
    inline bool containsBomb() const {
        return this->t == type::bomb_player || this->t == type::bomb;
    }
	inline bool containsPlayer() const {
        return this->t == type::bomb_player || this->t == type::player;
    }
    inline bool blocksExplosion() const {
        /*return (this->t == type::wall ||
				this->t == type::bomb ||
				this->t == type::bomb_player ||
				this->t == type::box ||
				this->t == type::box_b_range ||
				this->t == type::box_b_stock ||
				this->t == type::item_b_range ||
				this->t == type::item_b_stock) */
		return !(this->t == type::empty || this->t == type::player );
    }
    inline bool canBeDestroyed() const {
        if (!(this->t == type::empty || this->t == type::wall )
            ) {
            return true;
        }
        return false;
    }
    inline void explose() {
        if(this->t == type::box_b_range) {
           this->t = type::item_b_range;
        } else if(this->t == type::box_b_stock) {
           this->t = type::item_b_stock;
        } if(this->t != type::wall) {
           this->setEmpty();
        }
    }
    inline string toString() {
        return "location " + this->p.toString() + " type " + to_string(this->t);
    }
};

struct Gene {
    float move;
    bool bomb;

    inline Gene(Gene const&) = default;
    inline Gene(Gene&&) = default;
    inline Gene& operator=(Gene const&) = default;
    inline Gene& operator=(Gene&&) = default;

    inline Gene () {
        this->move = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
        if(static_cast <float> (rand()) / static_cast <float> (RAND_MAX) > 0.50){
            this->bomb = true;
        }else{
            this->bomb = false;
        }
    }
    inline Gene(float m, bool b) : move(m), bomb(b) {}

    inline void mutateMove() {
        this->move = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
    }
    inline void mutateBomb() {
        if (static_cast <float> (rand()) / static_cast <float> (RAND_MAX) > 0.50) {
            this->bomb = true;
        } else {
            this->bomb = false;
        }
    }

    inline string toString() const {
        string dir = ".";
        if (this->move <0.2) {
            dir = ".";
        } else if (this->move >=0.2 && this->move < 0.4) {
            dir = "RIGHT";
        } else if (this->move >= 0.4 && this->move < 0.6) {
            dir = "DOWN";
        } else if (this->move >= 0.6 && this->move < 0.8) {
            dir = "LEFT";
        } else if (this->move >= 0.8) {
            dir = "UP";
        }
        return "move: " + dir + " bomb: " + to_string(this->bomb);
    }
};

template <typename T>
struct myQueue
{
    T tab[1000]; // Should be more than enough
    uint first=0;
    uint next=0;

    inline myQueue(myQueue const&) = default;
    inline myQueue(myQueue&&) = default;
    inline myQueue& operator=(myQueue const&) = default;
    inline myQueue& operator=(myQueue&&) = default;

    inline myQueue() {
        for (uint i=0; i<1000; ++i) {
            tab[i] = NULL;
        }
        this->first=0;
        this->next=0;
    }

    inline bool empty() {
        return (this->next - this->first == 0);
    }
    inline T front() {
        return this->tab[this->first];
    }
    inline T front_and_pop() {
        ++this->first;
        return this->tab[this->first-1];
    }
    inline void pop() {
        ++this->first;
    }
    inline void push(T pBomb) {
        this->tab[next] = pBomb;
        ++this->next;
    }

};

template <typename T>
struct myList
{
    T tab[100]; // Should be more than enough
    uint first=0;
    uint next=0;

    inline myList(myList const&) = default;
    inline myList(myList&&) = default;
    inline myList& operator=(myList const&) = default;
    inline myList& operator=(myList&&) = default;

    inline myList() {
        for (uint i=0; i<100; ++i) {
            tab[i] = NULL;
        }
        this->first=0;
        this->next=0;
    }

    inline bool empty() {
        return (this->next - this->first == 0);
    }
    inline T getNext() {
        return this->tab[this->first];
        ++this->first;
    }
    inline void push_back(T pBomb) {
        this->tab[next] = pBomb;
        this->tab[next]->previous_bomb = &(this->tab[next-1]);
        this->tab[next-1]->next_bomb = &(this->tab[next]);
        ++this->next;
    }
    inline void remove(T pBomb) {
        pBomb->previous_bomb->next_bomb = pBomb->next_bomb;
        pBomb->next_bomb->previous_bomb = pBomb->previous_bomb;
        pBomb->previous_bomb = NULL;
        pBomb->next_bomb = NULL;
    }

};

struct Board
{
    Square theBoard[GLOBAL_MAX_WIDTH][GLOBAL_MAX_HEIGHT];
    Player players [GLOBAL_PLAYER_NUM];
    list<Bomb> bombs;
    int score;

    inline Board(Board const&) = default;
    inline Board(Board&&) = default;
    inline Board& operator=(Board const&) = default;
    inline Board& operator=(Board&&) = default;

    inline Board(){
        this->score = 0;
         for(char y= 0; y< GLOBAL_MAX_HEIGHT;++y){
            for(char x= 0; x< GLOBAL_MAX_WIDTH;++x){
               this->theBoard[x][y]=Square(Point(x, y));
            }
        }
        for(char i= 0; i< GLOBAL_PLAYER_NUM;++i){
           this->players[i] = Player(i,Point());
        }
    }
    inline Square get(int x, int y) {
        return this->theBoard[x][y];
    }
    inline list<char> getIDsOfPlayers() {
        list<char> res;
        for(char i=0; i< GLOBAL_PLAYER_NUM;++i){
           if ( !(this->players[i].p == Point()) ) {
               res.push_back(i);
           }
        }
        return res;
    }
    inline void init(int i, string row)
    {
        for(char x =0;x<GLOBAL_MAX_WIDTH;++x)
        {
            if (row[x] == '.') {
                this->theBoard[x][i].setEmpty();
            }else if (row[x] == 'X') {
                this->theBoard[x][i].addWall();
            } else {
                this->theBoard[x][i].addBox(row[x]);
            }
        }
    }
    inline void init(int entityType, int owner, int x, int y, int param1, int param2)
    {
        if (global_debug) cerr << "INIT : " << entityType << owner << x << y << param1 << param2 << endl;
        if (entityType == 0) { // Player
            this->theBoard[x][y].addPlayer();
            this->players[owner].update(owner,Point(x,y));
        } else if (entityType == 1) { //Bomb
            this->theBoard[x][y].addBomb();
            this->bombs.push_back(Bomb(owner, param2, param1, Point(x,y)));
        } else if (entityType == 2) { // Item
            this->theBoard[x][y].addItem(param1);
        }
    }
    inline void increaseScore(int n) {
        // Only increase if we are not dead
        this->score += n;
    }
    inline void killPlayersOnSquare(const Point& p) {
        for (char i=0; i<GLOBAL_PLAYER_NUM ; ++i) {
            if (this->players[i].p == p) {
                // Then player is dead
                this->players[i].kill();
            }
        }
    }
    inline void addBombToExplosionList(const Point & p, myQueue<Bomb*> &explosionList){
        // Add bombs in point that have timer > 0
        for (list<Bomb>::iterator it = this->bombs.begin(); it != this->bombs.end(); ++it) {
            if (it->p == p && it->timer > 0) {
                it->timer = 0;
                explosionList.push(&(*it));
            }
        }
    }
    inline bool processBomb(const Bomb & aBomb, myQueue<Bomb*> &explosionList, myQueue<Square*> &deletedObjects) {
        // Right
        for (char x=1; x < aBomb.range && aBomb.p.x+x < GLOBAL_MAX_WIDTH; ++x) {
            if (this->theBoard[aBomb.p.x+x][aBomb.p.y].canBeDestroyed()) {
                deletedObjects.push(&(this->theBoard[aBomb.p.x+x][aBomb.p.y]));
            }
            if (this->theBoard[aBomb.p.x+x][aBomb.p.y].blocksExplosion()) {
                if (this->theBoard[aBomb.p.x+x][aBomb.p.y].containsBomb()){
                    this->addBombToExplosionList(Point(aBomb.p.x+x,aBomb.p.y), explosionList);
                }
                if (this->theBoard[aBomb.p.x+x][aBomb.p.y].isBox()) {
                    // Give point to player
                    this->players[aBomb.owner].increaseScore();
                }
                break;
            }
        }
        // Left
        for (char x=1; x < aBomb.range && aBomb.p.x-x >= 0; ++x) {
            if (this->theBoard[aBomb.p.x-x][aBomb.p.y].canBeDestroyed()) {
                deletedObjects.push(&(this->theBoard[aBomb.p.x-x][aBomb.p.y]));
            }
            if (this->theBoard[aBomb.p.x-x][aBomb.p.y].blocksExplosion()) {
                if (this->theBoard[aBomb.p.x-x][aBomb.p.y].containsBomb()){
                    this->addBombToExplosionList(Point(aBomb.p.x-x,aBomb.p.y), explosionList);
                }
                if (this->theBoard[aBomb.p.x-x][aBomb.p.y].isBox()) {
                    // Give point to player
                    this->players[aBomb.owner].increaseScore();
                }
                break;
            }
        }
        // Down
        for (char y=1; y < aBomb.range && aBomb.p.y+y < GLOBAL_MAX_HEIGHT; ++y) {
            if (this->theBoard[aBomb.p.x][aBomb.p.y+y].canBeDestroyed()) {
                deletedObjects.push(&(this->theBoard[aBomb.p.x][aBomb.p.y+y]));
            }
            if (this->theBoard[aBomb.p.x][aBomb.p.y+y].blocksExplosion()) {
                if (this->theBoard[aBomb.p.x][aBomb.p.y+y].containsBomb()){
                    this->addBombToExplosionList(Point(aBomb.p.x,aBomb.p.y+y), explosionList);
                }
                if (this->theBoard[aBomb.p.x][aBomb.p.y+y].isBox()) {
                    // Give point to player
                    this->players[aBomb.owner].increaseScore();
                }
                break;
            }
        }
        // Up
        for (char y=1; y < aBomb.range && aBomb.p.y-y >= 0; ++y) {
            if (this->theBoard[aBomb.p.x][aBomb.p.y-y].canBeDestroyed()) {
                deletedObjects.push(&(this->theBoard[aBomb.p.x][aBomb.p.y-y]));
            }
            if (this->theBoard[aBomb.p.x][aBomb.p.y-y].blocksExplosion()) {
                if (this->theBoard[aBomb.p.x][aBomb.p.y-y].containsBomb()){
                    this->addBombToExplosionList(Point(aBomb.p.x,aBomb.p.y-y), explosionList);
                }
                if (this->theBoard[aBomb.p.x][aBomb.p.y-y].isBox()) {
                    // Give point to player
                    this->players[aBomb.owner].increaseScore();
                }
                break;
            }
        }
        //add bomb to player stock
        ++(this->players[aBomb.owner].reloading_stock);
        //remove bomb from board
        if (this->theBoard[aBomb.p.x][aBomb.p.y].containsPlayer()) {
            this->killPlayersOnSquare(aBomb.p);
        }
        deletedObjects.push(&(this->theBoard[aBomb.p.x][aBomb.p.y]));
        //remove bomb from list
        this->bombs.remove(aBomb);
        return false; // Default we suppose we are safe
    }
    inline void bigBadaboum() {
        //if (global_debug) cerr << "bigBadaboum " << this->bombs.size() << endl;
        // Go decrement all bomb timers
        myQueue<Bomb*> explosionList;
        myQueue<Square*> deletedObjects;
        for(list<Bomb>::iterator it = this->bombs.begin(); it != this->bombs.end();++it){
            it->tick();
            if (it->isExploding()) {
                explosionList.push(&(*it));
            }
        }

        // Simultaneous explosions
        while(!explosionList.empty()){
            //if (global_debug) cerr << "EXPLOSSSION " << endl;
            Bomb* pBomb = explosionList.front(); // Access 1st elem
            processBomb(*pBomb,explosionList, deletedObjects);
            explosionList.pop(); // Delete 1st elem
            //clean bomb list
        }
        // Cleaning the map
        while(!deletedObjects.empty()){
            Square* pSquare = deletedObjects.front();
            //if (global_debug) cerr << "Clean Board " << pSquare->toString() << endl;
            if (pSquare->containsPlayer()) {
                //if (global_debug) cerr << "Die you scum " << endl;
                this->killPlayersOnSquare(pSquare->p);
            }
            pSquare->explose();
            deletedObjects.pop();
        }
    }

    inline Point getNext(const Gene& g, const Point& p) const
    {
        Point pres(p);
        if (g.move <0.2) {
            return p;
        } else if (g.move >=0.2 && g.move < 0.4) {
            pres.x += 1;
        } else if (g.move >= 0.4 && g.move < 0.6) {
            pres.y += 1;
        } else if (g.move >= 0.6 && g.move < 0.8) {
            pres.x -= 1;
        } else if (g.move >= 0.8) {
            pres.y -= 1;
        }
        pres.correctBounds();
        if ( ! this->theBoard[pres.x][pres.y].canEnter() ) {
            return p;// Cannot move there, stay where we are
        }
        return pres;
    }
    // inline uint boxInRange(const Bomb& bomb){
    //     uint res = 0 ;
    //     for (char x = 0; x < bomb.range && bomb.p.x+x <=GLOBAL_MAX_WIDTH; ++x){
    //         if (global_debug) cerr << "Testing Case : " << this->theBoard[bomb.p.x+x][bomb.p.y].p.toString() << endl;
    //         if (global_debug) cerr << "Type Case : " << this->theBoard[bomb.p.x+x][bomb.p.y].t << endl;
    //         if (this->theBoard[bomb.p.x+x][bomb.p.y].t == Square::type::box){
    //             if (global_debug) cerr << "Box RIGHT: " << this->theBoard[bomb.p.x+x][bomb.p.y].p.toString() << endl;
    //             ++res;
    //             break;
    //         } else if (this->theBoard[bomb.p.x+x][bomb.p.y].t == Square::type::wall) {
    //             break;// A wall blocks the explosion, no need to look further
    //         }
    //     }


    //     for (char x = 0; x > -bomb.range && bomb.p.x+x >= 0; --x) {
    //         if (this->theBoard[bomb.p.x+x][bomb.p.y].t == Square::type::box){
    //               if (global_debug) cerr << "Box LEFT: " << this->theBoard[bomb.p.x+x][bomb.p.y].p.toString() << endl;
    //             ++res;
    //             break;
    //         } else if (this->theBoard[bomb.p.x+x][bomb.p.y].t == Square::type::wall) {
    //             break;// A wall blocks the explosion, no need to look further
    //         }
    //     }

    //     for (char y = 0; y < bomb.range && bomb.p.y+y <= GLOBAL_MAX_HEIGHT; ++y){
    //         if (this->theBoard[bomb.p.x][bomb.p.y+y].t == Square::type::box){
    //             if (global_debug) cerr << "Box DOWN: " << this->theBoard[bomb.p.x][bomb.p.y+y].p.toString() << endl;
    //             ++res;
    //             break;
    //         } else if (this->theBoard[bomb.p.x][bomb.p.y+y].t == Square::type::wall) {
    //             break;// A wall blocks the explosion, no need to look further
    //         }
    //     }

    //     for (char y = 0; y > -bomb.range && bomb.p.y+y >= 0; --y){
    //         if (this->theBoard[bomb.p.x][bomb.p.y+y].t == Square::type::box){
    //             if (global_debug) cerr << "Box UP: " << this->theBoard[bomb.p.x][bomb.p.y+y].p.toString() << endl;
    //             ++res;
    //             break;
    //         } else if (this->theBoard[bomb.p.x][bomb.p.y+y].t == Square::type::wall) {
    //             break;// A wall blocks the explosion, no need to look further
    //         }
    //     }
    //     return res;
    // }


    inline void update(const int& id, const Gene& g, int multiplier){

        // cf. Experts rules for details
        // First: bombs explodes (if reach timer 0) and destroy objects
        int score_inc = this->players[id].score;
        this->bigBadaboum();
        score_inc = this->players[id].score - score_inc;
        if(this->players[id].isAlive){
            // Treat the bomb dropped case TODO include in bigBadaboum
            //if (global_debug) {cerr << "Stock before planting " << to_string(this->players[id].cur_stock) << endl;}
            if (g.bomb && this->players[id].cur_stock > 0 && !this->theBoard[this->players[id].p.x][this->players[id].p.y].containsBomb()) {
                // Add bomb on the square and in the list of bombs too
                this->addBomb(this->players[id]);
                this->increaseScore(-1);
            }
            //if (global_debug) {cerr << "Stock after planting " << to_string(this->players[id].cur_stock) << endl;}

            this->increaseScore(score_inc * multiplier);
            //if (global_debug) cerr << "Player is: " << this->players[myId].p.toString() << endl;
            //if (global_debug) cerr << "Square ok: " << this->theBoard[this->players[myId].p.x][this->players[myId].p.y].containsPlayer() << endl;
            //if (global_debug) cerr << "score " << score_inc << " boxes with multiplier " << multiplier << endl;
            // Then: we move the player(s)
            Point new_pos = this->getNext(g, this->players[id].p);
            if(!(new_pos == this->players[id].p)){
                // Treat the movement of the player
                if ((this->theBoard[new_pos.x][new_pos.y].canEnter()) &&
                   new_pos.x >= 0 && new_pos.x < GLOBAL_MAX_WIDTH &&
                   new_pos.y >= 0 && new_pos.y < GLOBAL_MAX_HEIGHT) { // valid move
                    if (this->theBoard[new_pos.x][new_pos.y].hasBonus()) { // we take an item
                       this->increaseScore(1 * (multiplier/2));
                       if(this->theBoard[new_pos.x][new_pos.y].t == Square::type::item_b_range){
                           ++this->players[id].range;
                       }else{
                           ++this->players[id].cur_stock;
                       }
                       this->theBoard[new_pos.x][new_pos.y].removeBonus();
                    }
                    // update the new square with the player information
                    this->theBoard[this->players[id].p.x][this->players[id].p.y].removePlayer();
                    this->theBoard[new_pos.x][new_pos.y].addPlayer();
                    //update the player
                    this->players[id].p.x = new_pos.x;
                    this->players[id].p.y = new_pos.y;
                }
            }
            this->players[id].reload();
            //if (global_debug) {cerr << "Player moved " << this->players[myId].p.toString() << endl;}
        }else {
            this->score = INT_MIN;
        }
    }

    inline void addBomb(Player& pyro){
        --(pyro.cur_stock);
        this->bombs.push_back(Bomb(pyro.id, pyro.range, 8 /* Timer 8 for all new bombs */, pyro.p));
        this->theBoard[pyro.p.x][pyro.p.y].addBomb();
    }

    inline void toString(){
        for(char y= 0; y< GLOBAL_MAX_HEIGHT;++y){
            for(char x= 0; x< GLOBAL_MAX_WIDTH;++x){
                cerr << this->theBoard[x][y].t <<" ";
            }
            cerr << endl;
        }
    }
};

struct Genome {
    int score = 0;
    Gene array[GLOBAL_GENOME_SIZE];
    inline Genome() = default;
    inline Genome(Genome const&) = default;
    inline Genome(Genome&&) = default;
    inline Genome& operator=(Genome const&) = default;
    inline Genome& operator=(Genome&&) = default;
    inline bool operator<(const Genome& g2) const {
        if (this->score < g2.score) {
            return true;
        } else {
            return false;
        }
    }
    inline string toString() const {
        string res = "";
        for (int i =0;i<GLOBAL_GENOME_SIZE;++i){
            res += this->array[i].toString() + "\n";
        }
        return res;
    }
};



int calculateScore(const int& id,const Genome& genome, Board board)
{
    for (char i=0; i<GLOBAL_GENOME_SIZE; ++i) {
        //if (global_debug) {cerr << "We update with gene: " << genome.array[i].toString() << endl;}
        board.update(id, genome.array[i], GLOBAL_GENOME_SIZE-i);
        //if (global_debug) {cerr << "New score: " << board.score << endl;}
        if(board.score == INT_MIN) {
            break;
        }
    }
    return board.score;
}


struct Evolution {
    priority_queue<Genome> theGenomes;

    inline Evolution() = default;
    inline Evolution(Evolution const&) = default;
    inline Evolution(Evolution&&) = default;
    inline Evolution& operator=(Evolution const&) = default;
    inline Evolution& operator=(Evolution&&) = default;

    inline Evolution(const int& id, uint max) {
        for (uint i=0; i<max && !(global_timer->isTimesUp()); ++i) {
            calculateScoreAndInsert(id, Genome());
        }
    }

    inline Gene crossGenes(const Gene& g1, const Gene& g2) {
        float gene_proba = static_cast <float> (rand()) / static_cast <float> (RAND_MAX) ;
        if (gene_proba < 0.4) {
            // Keep g1
            return Gene(g1);
        } else if (gene_proba < 0.8) {
            // Keep g2
            return Gene(g2);
        } else if (gene_proba < 0.85) {
            return Gene(g1.move * g2.move, g1.bomb || g2.bomb);
        } else if (gene_proba < 0.90) {
            return Gene(g1.move * g2.move, g1.bomb && g2.bomb);
        } else {
            return Gene();
        }
    }

    inline void crossGenenome(const Genome& g1, const Genome& g2, Genome& gres) {
        for (char i=0; i<GLOBAL_GENOME_SIZE; ++i) {
            gres.array[i] = this->crossGenes(g1.array[i], g2.array[i]);
        }
    }

    inline void calculateScoreAndInsert(const int& id, Genome g) {// Not sure about putting a ref here or not
        g.score = calculateScore(id, g, *global_board);
        this->theGenomes.push(g);
        ++global_compute;
    }

    inline void evolveOnce(const int & id) {
        Genome theNewGenomes[GLOBAL_POPULATION_SIZE];
        Genome bestGenomes[GLOBAL_POPULATION_SIZE/100];
        uint last = 0; // last = GLOBAL_POPULATION_SIZE/10 (best) + 8*GLOBAL_POPULATION_SIZE/10 (crossed) + GLOBAL_POPULATION_SIZE/10 (mutant)
        // Keep the 10 best percent
        //cerr << "Keep the 10 best percent " << endl;
        while (!theGenomes.empty() && last < GLOBAL_POPULATION_SIZE/100) {
            bestGenomes[last] = theGenomes.top(); // Get elem
            ++last;
            theGenomes.pop(); // Delete it
        }
        this->theGenomes = priority_queue<Genome>();
        //insert old best into queue
        //cerr << "insert old best into queue " << endl;
        for (uint i=0; i< last; ++i) {
            this->theGenomes.push(bestGenomes[i]);
        }
        //TODO may be add pure random gene
        for (uint i=0; i<GLOBAL_POPULATION_SIZE/4; ++i) {
            // Add 20% of random
            this->calculateScoreAndInsert(id, Genome());
            ++last;
        }
        //cerr << "generate new pop " << endl;
        for (/*last alredy set*/; last < GLOBAL_POPULATION_SIZE && !(global_timer->isTimesUp()); ++last) {
            int index_genome1 = int ((static_cast <float> (rand()) / static_cast <float> (RAND_MAX)) * (GLOBAL_POPULATION_SIZE/100)) ;
            int index_genome2 = int((static_cast <float> (rand()) / static_cast <float> (RAND_MAX)) * (GLOBAL_POPULATION_SIZE/100)) ;
            this->crossGenenome(bestGenomes[index_genome1], bestGenomes[index_genome2], theNewGenomes[last]);
            // We have a new genome with a new score
            this->calculateScoreAndInsert(id, theNewGenomes[last]);
        }
    }

    inline void evolve(const int& id) {
        while(!(global_timer->isTimesUp())){
            this->evolveOnce(id);
        }
    }
};

string output(const int& id, const Gene& g, const Board& b){
    string res = "";
    if (g.bomb) {
        res += "BOMB";
    } else {
        res += "MOVE";
    }
    res += " ";
    res += b.getNext(g, b.players[id].p).toString();
    return res;
}

/**
 * Auto-generated code below aims at helping you parse
 * the standard input according to the problem statement.
 **/
int main()
{
    bool first_turn = false;
    int width;
    int height;
    double score_cumul = 0;
    int turn = 1;
    int myId;
    cin >> width >> height >> myId; cin.ignore();
    Board theBoard = Board();
    global_board = &theBoard;
    // game loop
    while (1)
    {
        global_debug=false;
        for (int i = 0; i < height; i++)
        {
            string row;
            getline(cin, row);
            global_board->init(i,row);
        }
        int entities;
        cin >> entities; cin.ignore();
        global_board->bombs.clear();
        global_board->score=0;
        for (int i = 0; i < entities; i++) {
            int entityType;
            int owner;
            int x;
            int y;
            int param1;
            int param2;
            cin >> entityType >> owner >> x >> y >> param1 >> param2; cin.ignore();
            global_board->init(entityType, owner, x, y, param1, param2);
        }
        Timer timer = Timer(first_turn);
        global_timer = &timer;
        //global_board->toString();
        global_debug=false;
        Genome bestGenomes[GLOBAL_PLAYER_NUM];
        list<char> playersID = global_board->getIDsOfPlayers();
        // TODO uncomment to calculate scores for others
        /*for (list<char>::const_iterator it = playersID.end(); it != playersID.end(); ++it) {
            if (*it != myId) {
                // Calculate genomes for others also
                bestGenomes[*it] = Evolution(*it, 1000).theGenomes.top(); // Does not evolve, only random here at first
            }
        }*/
        Evolution evol(myId, GLOBAL_POPULATION_SIZE*3);
        evol.evolve(myId);
        bestGenomes[myId] = evol.theGenomes.top();// Evolve as much as possible
        const Genome& myBestGenome = bestGenomes[myId];
        int bestScore=INT_MIN;
        global_debug=true;
        //cerr << "PLayer" << global_board->players[global_board->myId].toString() << endl;
        bestScore = calculateScore(myId, myBestGenome, *global_board);
        global_debug=false;
        //cerr << "Best score selected is " << myBestGenome.score << endl;
        //cerr << myBestGenome.toString() << endl;
        // Write an action using cout. DON'T FORGET THE "<< endl"
        // To debug: cerr << "Debug messages..." << endl;

        cout << output(myId, myBestGenome.array[0], *global_board) << endl;
        score_cumul += global_compute;
        cerr << "average:" << (score_cumul/turn) << endl;
        //apply first gene of best genome to the board
        global_board->update(myId,myBestGenome.array[0], GLOBAL_GENOME_SIZE);
        ++turn;
        global_compute = 0;
    }
}
