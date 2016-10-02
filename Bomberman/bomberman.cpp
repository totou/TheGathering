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
const uint GLOBAL_POPULATION_SIZE = 2500;
const uint GLOBAL_MAX_GENERATION_NUM = 50;
const uint GLOBAL_INITIAL_POP_SIZE = GLOBAL_POPULATION_SIZE*4;
const uint GLOBAL_TOP_PERCENT = GLOBAL_POPULATION_SIZE/100;// Top 1%

int global_turn = 1;

uint global_compute = 0;
uint global_generation = 0;
struct Board;
Board* global_board;

struct Timer{
    chrono::time_point<chrono::system_clock> end;
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
        this->isAlive = false;
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
               " reloading_stock: " + to_string(this->reloading_stock) +
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
    enum type { empty, bomb, box, box_b_range, box_b_stock, item_b_range,item_b_stock, wall};
    type t;
    Point p;
    char hasPlayer;

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
			this->t = type::bomb;
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
        this->hasPlayer = 0;
    }
    inline bool canEnter() const {
        return ((this->t == type::empty ||
				 this->t == type::item_b_range ||
				 this->t == type::item_b_stock )&& this->hasPlayer == 0);

    }
    inline bool isBox() const {
        return (this->t == type::box ||
				this->t == type::box_b_range ||
				this->t == type::box_b_stock);
    }
    inline bool isBonusBox() const {
        return (this->t == type::box_b_range ||
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
		++this->hasPlayer;
    }
    inline void removePlayer() {
        --this->hasPlayer;
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
        return this->t == type::bomb;
    }
	inline bool containsPlayer() const {
        return this->hasPlayer > 0;
    }
    inline bool blocksExplosion() const {
		return !(this->t == type::empty );
    }
    inline bool canBeDestroyed() const {
        return !((this->t == type::empty || this->t == type::wall) && this->hasPlayer == 0 );
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
    inline string toString() const {
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
    inline void update(float m, bool b){
        this->move = m;
        this->bomb = b;
    }

    inline void mutateMove() {
        this->move = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
    }
    inline void mutateBomb() {
        if (static_cast <float> (rand()) / static_cast <float> (RAND_MAX) > 0.5) {
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

    inline void cross(const Gene& g1, const Gene& g2) {
        float gene_proba = static_cast <float> (rand()) / static_cast <float> (RAND_MAX) ;
        if (gene_proba < 0.4) {
            // Keep g1
            *this = g1;
        } else if (gene_proba < 0.8) {
            // Keep g2
            *this = g2;
        } else if (gene_proba < 0.85) {
            this->update(g1.move * g2.move, g1.bomb || g2.bomb);
        } else if (gene_proba < 0.90) {
            this->update(g1.move * g2.move, g1.bomb  &&  g2.bomb);
        } else {
            *this = Gene();
        }
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
        this->first=0;
        this->next=0;
    }
    inline void setEmpty() {
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

char g_board_i;
char g_board_x;
char g_board_y;
char g_board_init_x;
char g_board_killPlayersOnSquare_i;
char g_board_processBomb_x;
char g_board_processBomb_y;
char g_board_update_i;

int g_board_update_score_inc;
int g_board_player_temp_score [GLOBAL_PLAYER_NUM];
Point g_board_newPositions [GLOBAL_PLAYER_NUM];

myQueue<Bomb*> g_board_explosionList;
myQueue<Square*> g_board_deletedObjects;
myQueue<Square*> g_board_deleteBox;

struct Board
{
    Square theBoard[GLOBAL_MAX_WIDTH][GLOBAL_MAX_HEIGHT];
    Player players [GLOBAL_PLAYER_NUM];
    list<Bomb> bombs;
    int scores [GLOBAL_PLAYER_NUM];

    inline Board(Board const&) = default;
    inline Board(Board&&) = default;
    inline Board& operator=(Board const&) = default;
    inline Board& operator=(Board&&) = default;

    inline Board(){
        for(g_board_i= 0; g_board_i< GLOBAL_PLAYER_NUM;++g_board_i){
            this->scores[g_board_i] = 0;
        }
        for(g_board_y= 0; g_board_y< GLOBAL_MAX_HEIGHT;++g_board_y){
            for(g_board_x= 0; g_board_x< GLOBAL_MAX_WIDTH;++g_board_x){
               this->theBoard[g_board_x][g_board_y]=Square(Point(g_board_x, g_board_y));
            }
        }
        for(g_board_i= 0; g_board_i< GLOBAL_PLAYER_NUM;++g_board_i){
           this->players[g_board_i] = Player(g_board_i,Point());
        }
    }
    inline Square get(int x, int y) {
        return this->theBoard[x][y];
    }
    inline void init(int i, string row)
    {
        for(g_board_init_x =0;g_board_init_x<GLOBAL_MAX_WIDTH;++g_board_init_x)
        {
            if (row[g_board_init_x] == '.') {
                this->theBoard[g_board_init_x][i].setEmpty();
            }else if (row[g_board_init_x] == 'X') {
                this->theBoard[g_board_init_x][i].addWall();
            } else {
                this->theBoard[g_board_init_x][i].addBox(row[g_board_init_x]);
            }
        }
    }
    inline void init(int entityType, int owner, int x, int y, int param1, int param2, const Board& previous_board)
    {
        if (entityType == 0) { // Player
            if(global_turn == 1){
                this->players[owner].isAlive = true;
            }
            if (this->players[owner].isAlive){
                this->theBoard[x][y].addPlayer();
                this->players[owner].update(owner, Point(x, y));
                if (previous_board.theBoard[x][y].hasBonus()) { // we take an item
                    if(previous_board.theBoard[x][y].t == Square::type::item_b_range){
                        ++this->players[owner].range;
                    }else{
                        ++this->players[owner].cur_stock;
                    }
                }
                this->players[owner].reload();
            }
        } else if (entityType == 1) { //Bomb
            this->theBoard[x][y].addBomb();
            this->bombs.push_back(Bomb(owner, param2, param1, Point(x,y)));
            if(!(previous_board.theBoard[x][y].containsBomb())){
                --this->players[owner].cur_stock;
            }
        } else if (entityType == 2) { // Item
            this->theBoard[x][y].addItem(param1);
        }
    }
    inline void increaseScore(int n, const char& id) {
        // Only increase if we are not dead
        this->scores[id] += n;
    }
    inline void killPlayersOnSquare(const Point& p) {
        for (g_board_killPlayersOnSquare_i=0; g_board_killPlayersOnSquare_i<GLOBAL_PLAYER_NUM ; ++g_board_killPlayersOnSquare_i) {
            if (this->players[g_board_killPlayersOnSquare_i].p == p) {
                // Then player is dead
                this->players[g_board_killPlayersOnSquare_i].kill();
                //if (global_debug) cerr << "Die " << to_string(g_board_killPlayersOnSquare_i) << endl;
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
        for (g_board_processBomb_x=1; g_board_processBomb_x < aBomb.range && aBomb.p.x+g_board_processBomb_x < GLOBAL_MAX_WIDTH; ++g_board_processBomb_x) {
            if (this->theBoard[aBomb.p.x+g_board_processBomb_x][aBomb.p.y].canBeDestroyed()) {
                deletedObjects.push(&(this->theBoard[aBomb.p.x+g_board_processBomb_x][aBomb.p.y]));
                if(this->theBoard[aBomb.p.x+g_board_processBomb_x][aBomb.p.y].containsPlayer()){
                    this->players[aBomb.owner].increaseScore();
                }
            }
            if (this->theBoard[aBomb.p.x+g_board_processBomb_x][aBomb.p.y].blocksExplosion()) {
                if (this->theBoard[aBomb.p.x+g_board_processBomb_x][aBomb.p.y].containsBomb()){
                    this->addBombToExplosionList(Point(aBomb.p.x+g_board_processBomb_x,aBomb.p.y), explosionList);
                }
                if (this->theBoard[aBomb.p.x+g_board_processBomb_x][aBomb.p.y].isBox()) {
                    // Give point to player
                    this->players[aBomb.owner].increaseScore();
                    // if (this->theBoard[aBomb.p.x+x][aBomb.p.y].isBonusBox()) {
                        // this->score += 1;// Small incentive to destroy bonus boxes compared to normal ones
                    // }
                }
                break;
            }
        }
        // Left
        for (g_board_processBomb_x=1; g_board_processBomb_x < aBomb.range && aBomb.p.x-g_board_processBomb_x >= 0; ++g_board_processBomb_x) {
            if (this->theBoard[aBomb.p.x-g_board_processBomb_x][aBomb.p.y].canBeDestroyed()) {
                deletedObjects.push(&(this->theBoard[aBomb.p.x-g_board_processBomb_x][aBomb.p.y]));
                if(this->theBoard[aBomb.p.x-g_board_processBomb_x][aBomb.p.y].containsPlayer()){
                    this->players[aBomb.owner].increaseScore();
                }
            }
            if (this->theBoard[aBomb.p.x-g_board_processBomb_x][aBomb.p.y].blocksExplosion()) {
                if (this->theBoard[aBomb.p.x-g_board_processBomb_x][aBomb.p.y].containsBomb()){
                    this->addBombToExplosionList(Point(aBomb.p.x-g_board_processBomb_x,aBomb.p.y), explosionList);
                }
                if (this->theBoard[aBomb.p.x-g_board_processBomb_x][aBomb.p.y].isBox()) {
                    // Give point to player
                    this->players[aBomb.owner].increaseScore();
                    // if (this->theBoard[aBomb.p.x-x][aBomb.p.y].isBonusBox()) {
                        // this->score += 1;// Small incentive to destroy bonus boxes compared to normal ones
                    // }
                }
                break;
            }
        }
        // Down
        for (g_board_processBomb_y=1; g_board_processBomb_y < aBomb.range && aBomb.p.y+g_board_processBomb_y < GLOBAL_MAX_HEIGHT; ++g_board_processBomb_y) {
            if (this->theBoard[aBomb.p.x][aBomb.p.y+g_board_processBomb_y].canBeDestroyed()) {
                deletedObjects.push(&(this->theBoard[aBomb.p.x][aBomb.p.y+g_board_processBomb_y]));
                if(this->theBoard[aBomb.p.x][aBomb.p.y+g_board_processBomb_y].containsPlayer()){
                    this->players[aBomb.owner].increaseScore();
                }
            }
            if (this->theBoard[aBomb.p.x][aBomb.p.y+g_board_processBomb_y].blocksExplosion()) {
                if (this->theBoard[aBomb.p.x][aBomb.p.y+g_board_processBomb_y].containsBomb()){
                    this->addBombToExplosionList(Point(aBomb.p.x,aBomb.p.y+g_board_processBomb_y), explosionList);
                }
                if (this->theBoard[aBomb.p.x][aBomb.p.y+g_board_processBomb_y].isBox()) {
                    // Give point to player
                    this->players[aBomb.owner].increaseScore();
                    // if (this->theBoard[aBomb.p.x][aBomb.p.y+y].isBonusBox()) {
                        // this->score += 1;// Small incentive to destroy bonus boxes compared to normal ones
                    // }
                }
                break;
            }
        }
        // Up
        for (g_board_processBomb_y=1; g_board_processBomb_y < aBomb.range && aBomb.p.y-g_board_processBomb_y >= 0; ++g_board_processBomb_y) {
            if (this->theBoard[aBomb.p.x][aBomb.p.y-g_board_processBomb_y].canBeDestroyed()) {
                deletedObjects.push(&(this->theBoard[aBomb.p.x][aBomb.p.y-g_board_processBomb_y]));
                if(this->theBoard[aBomb.p.x][aBomb.p.y-g_board_processBomb_y].containsPlayer()){
                    this->players[aBomb.owner].increaseScore();
                }
            }
            if (this->theBoard[aBomb.p.x][aBomb.p.y-g_board_processBomb_y].blocksExplosion()) {
                if (this->theBoard[aBomb.p.x][aBomb.p.y-g_board_processBomb_y].containsBomb()){
                    this->addBombToExplosionList(Point(aBomb.p.x,aBomb.p.y-g_board_processBomb_y), explosionList);
                }
                if (this->theBoard[aBomb.p.x][aBomb.p.y-g_board_processBomb_y].isBox()) {
                    // Give point to player
                    this->players[aBomb.owner].increaseScore();
                    // if (this->theBoard[aBomb.p.x][aBomb.p.y-y].isBonusBox()) {
                        // this->score += 1;// Small incentive to destroy bonus boxes compared to normal ones
                    // }
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
    inline void bigBadaboum(myQueue<Square*>& deleteBox) {
        //if (global_debug) cerr << "bigBadaboum " << this->bombs.size() << endl;
        // Go decrement all bomb timers
		g_board_explosionList.setEmpty();
		g_board_deletedObjects.setEmpty();
        for(list<Bomb>::iterator it = this->bombs.begin(); it != this->bombs.end();++it){
            it->tick();
            if (it->isExploding()) {
                g_board_explosionList.push(&(*it));
            }
        }

        // Simultaneous explosions
        while(!g_board_explosionList.empty()){
            Bomb* pBomb = g_board_explosionList.front(); // Access 1st elem
            processBomb(*pBomb,g_board_explosionList, g_board_deletedObjects);
            g_board_explosionList.pop(); // Delete 1st elem
            //clean bomb list
        }
        // Cleaning the map
        while(!g_board_deletedObjects.empty()){
            Square* pSquare = g_board_deletedObjects.front();
            //if (global_debug) cerr << "Clean Board " << pSquare->toString() << endl;
            if (pSquare->containsPlayer()) {
                this->killPlayersOnSquare(pSquare->p);
            }
            if (!pSquare->isBox()) {
                pSquare->explose();
            } else {
                deleteBox.push(pSquare);
            }
            g_board_deletedObjects.pop();
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

    inline Point getNextWithoutCheck(const Gene& g, const Point& p) const
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
        return pres;
    }
    inline void update(const Gene genes[GLOBAL_PLAYER_NUM] , int multiplier){

        // cf. Experts rules for details
        // First: bombs explodes (if reach timer 0) and destroy objects
        for(g_board_update_i = 0; g_board_update_i < GLOBAL_PLAYER_NUM;++g_board_update_i){
            g_board_player_temp_score[g_board_update_i] = this->players[g_board_update_i].score;
        }
		g_board_deleteBox.setEmpty();
        this->bigBadaboum(g_board_deleteBox);
        for(g_board_update_i = 0; g_board_update_i < GLOBAL_PLAYER_NUM;++g_board_update_i){
            if(this->players[g_board_update_i].isAlive){
                g_board_newPositions[g_board_update_i] = this->getNext(genes[g_board_update_i], this->players[g_board_update_i].p);
                //if (global_debug) cerr << "P " << to_string(g_board_update_i) << "new " << g_board_newPositions[g_board_update_i].toString() << endl;
            }
        }
        for(g_board_update_i = 0; g_board_update_i < GLOBAL_PLAYER_NUM;++g_board_update_i){
            g_board_update_score_inc = this->players[g_board_update_i].score - g_board_player_temp_score[g_board_update_i];
            if(this->players[g_board_update_i].isAlive){
                // Treat the bomb dropped case TODO include in bigBadaboum
                //if (global_debug) {cerr << "Stock before planting " << to_string(this->players[id].cur_stock) << endl;}
                if (genes[g_board_update_i].bomb && this->players[g_board_update_i].cur_stock > 0 && !this->theBoard[this->players[g_board_update_i].p.x][this->players[g_board_update_i].p.y].containsBomb()) {
                    // Add bomb on the square and in the list of bombs too
                    this->addBomb(this->players[g_board_update_i]);
                    this->increaseScore(-1,g_board_update_i);
                }
                //if (global_debug) {cerr << "Stock after planting " << to_string(this->players[id].cur_stock) << endl;}

                this->increaseScore(g_board_update_score_inc * multiplier,g_board_update_i);
                //if (global_debug) cerr << "Player is: " << this->players[myId].p.toString() << endl;
                //if (global_debug) cerr << "Square ok: " << this->theBoard[this->players[myId].p.x][this->players[myId].p.y].containsPlayer() << endl;
                //if (global_debug) cerr << "score " << g_board_update_score_inc << " boxes with multiplier " << multiplier << endl;
                // Then: we move the player(s)
                if(!(g_board_newPositions[g_board_update_i] == this->players[g_board_update_i].p)){
                    // Treat the movement of the player
                    if ((this->theBoard[g_board_newPositions[g_board_update_i].x][g_board_newPositions[g_board_update_i].y].canEnter()) &&
                       g_board_newPositions[g_board_update_i].x >= 0 && g_board_newPositions[g_board_update_i].x < GLOBAL_MAX_WIDTH &&
                       g_board_newPositions[g_board_update_i].y >= 0 && g_board_newPositions[g_board_update_i].y < GLOBAL_MAX_HEIGHT) { // valid move
                        if (this->theBoard[g_board_newPositions[g_board_update_i].x][g_board_newPositions[g_board_update_i].y].hasBonus()) { // we take an item
                           this->increaseScore(1*multiplier,g_board_update_i);
                           if(this->theBoard[g_board_newPositions[g_board_update_i].x][g_board_newPositions[g_board_update_i].y].t == Square::type::item_b_range){
                               ++this->players[g_board_update_i].range;
                           }else{
                               ++this->players[g_board_update_i].cur_stock;
                           }
                           this->theBoard[g_board_newPositions[g_board_update_i].x][g_board_newPositions[g_board_update_i].y].removeBonus();
                        }
                        // update the new square with the player information
                        // int i = g_board_update_i;
                        // if (global_debug) cerr << "P" << i << " old " << this->players[g_board_update_i].p << endl;
                        //if (global_debug) cerr << "P " << to_string(g_board_update_i) << "old " << this->players[g_board_update_i].p.toString() << endl;
                        this->theBoard[this->players[g_board_update_i].p.x][this->players[g_board_update_i].p.y].removePlayer();
                        this->theBoard[g_board_newPositions[g_board_update_i].x][g_board_newPositions[g_board_update_i].y].addPlayer();
                        //update the player
                        this->players[g_board_update_i].p.x = g_board_newPositions[g_board_update_i].x;
                        this->players[g_board_update_i].p.y = g_board_newPositions[g_board_update_i].y;
                    }
                }
                this->players[g_board_update_i].reload();
                //if (global_debug) {cerr << "Player moved " << this->players[myId].p.toString() << endl;}
            }else {
                this->scores[g_board_update_i] = INT_MIN;
            }
		}
		// Clean boxes
        while (!g_board_deleteBox.empty()) {
            Square* pSquare = g_board_deleteBox.front();
            //if (global_debug) cerr << "Clean Board " << pSquare->toString() << endl;
            pSquare->explose();
            g_board_deleteBox.pop();
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
                cerr << this->theBoard[x][y].t + 10 * (this->theBoard[x][y].hasPlayer+1) <<" ";
            }
            cerr << endl;
        }
    }
};

char g_genome_i;
struct Genome {
    int score = INT_MIN;
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
    inline void updateFrom(const Genome& previous) {
        for (int i=0; i<GLOBAL_GENOME_SIZE-1; ++i){
            this->array[i] = previous.array[i+1];
        }
        this->array[GLOBAL_GENOME_SIZE-1] = Gene();
    }
    inline string toString() const {
        string res = "";
        for (int i =0;i<GLOBAL_GENOME_SIZE;++i){
            res += this->array[i].toString() + "\n";
        }
        return res;
    }
    inline void nextGen(){
        for(g_genome_i=0;g_genome_i<GLOBAL_GENOME_SIZE-1;++g_genome_i){
            this->array[g_genome_i] = this->array[g_genome_i+1] ;
        }
        this->array[GLOBAL_GENOME_SIZE-1] = Gene();
    }
    inline void cross(const Genome& g1, const Genome& g2) {
        for (g_genome_i=0; g_genome_i<GLOBAL_GENOME_SIZE; ++g_genome_i) {
            this->array[g_genome_i].cross(g1.array[g_genome_i],g2.array[g_genome_i]);
        }
    }
};

char g_TopPercentGenome_i;
char g_TopPercentGenome_best;

struct TopPercentGenome {
    Genome array[GLOBAL_TOP_PERCENT];
    char minIt = 0;
    inline TopPercentGenome() = default;
    inline TopPercentGenome(TopPercentGenome const&) = default;
    inline TopPercentGenome(TopPercentGenome&&) = default;
    inline TopPercentGenome& operator=(TopPercentGenome const&) = default;
    inline TopPercentGenome& operator=(TopPercentGenome&&) = default;

    inline void addSup(const Genome& g) {
        if( this->array[this->minIt] < g ){
            this->array[this->minIt] = g;
            for(g_TopPercentGenome_i=0;g_TopPercentGenome_i<GLOBAL_TOP_PERCENT;++g_TopPercentGenome_i){
                if(this->array[g_TopPercentGenome_i] < this->array[this->minIt]){
                    this->minIt = g_TopPercentGenome_i;
                }
            }
        }
    }
    inline Genome top() {
        g_TopPercentGenome_best = 0;
        for(g_TopPercentGenome_i=1;g_TopPercentGenome_i<GLOBAL_TOP_PERCENT;++g_TopPercentGenome_i){
            if(this->array[g_TopPercentGenome_best] < this->array[g_TopPercentGenome_i]){
                g_TopPercentGenome_best = g_TopPercentGenome_i;
            }
        }
        return this->array[g_TopPercentGenome_best];
    }
};

char g_FullGenome_i;
struct FullGenome {
    Genome array[GLOBAL_PLAYER_NUM];
    inline FullGenome() = default;
    inline FullGenome(FullGenome const&) = default;
    inline FullGenome(FullGenome&&) = default;
    inline FullGenome& operator=(FullGenome const&) = default;
    inline FullGenome& operator=(FullGenome&&) = default;

    inline FullGenome(const Genome& g1,const Genome& g2,const Genome& g3,const Genome& g4) {
        this->array[0] = g1;
        this->array[1] = g2;
        this->array[2] = g3;
        this->array[3] = g4;
    }

    inline void update(const int& id,const Genome& g) {
        this->array[id] = g;
    }
    inline void genes(const int& id, Gene gArray[GLOBAL_PLAYER_NUM]){
        for(g_FullGenome_i = 0; g_FullGenome_i<GLOBAL_PLAYER_NUM;++g_FullGenome_i){
            gArray[g_FullGenome_i]= this->array[g_FullGenome_i].array[id];
        }
    }

    inline void nextGen(){
        for(g_FullGenome_i=0;g_FullGenome_i<GLOBAL_PLAYER_NUM;++g_FullGenome_i){
            this->array[g_FullGenome_i].nextGen();
        }
    }
    inline void cross(const FullGenome& g1, const FullGenome& g2){
        for(g_FullGenome_i=0;g_FullGenome_i<GLOBAL_PLAYER_NUM;++g_FullGenome_i){
            this->array[g_FullGenome_i].cross(g1.array[g_FullGenome_i],g2.array[g_FullGenome_i]);
        }
    }
};

struct Evolution {
    FullGenome theFullGenomes [GLOBAL_POPULATION_SIZE];
    TopPercentGenome theTopGenomes [GLOBAL_PLAYER_NUM];

    inline Evolution() = default;
    inline Evolution(Evolution const&) = default;
    inline Evolution(Evolution&&) = default;
    inline Evolution& operator=(Evolution const&) = default;
    inline Evolution& operator=(Evolution&&) = default;

    inline Evolution(const int& id, uint max, const FullGenome& bestFullGenomes) {
        calculateScoreAndReplace(id,bestFullGenomes);
        for (uint i=1; i<max && !(global_timer->isTimesUp()); ++i) {
            calculateScoreAndReplace(id, FullGenome());
        }
    }

    inline void calculateScore(const int& id, FullGenome & genomes, Board board)
    {
        char i;
        for (i=0; i<GLOBAL_GENOME_SIZE; ++i) {
            Gene gArray[GLOBAL_PLAYER_NUM];
            genomes.genes(i, gArray);
            board.update(gArray, GLOBAL_GENOME_SIZE-i);
            if(board.scores[id] == INT_MIN) {
                break;
            }
        }
        for (i=0; i<GLOBAL_PLAYER_NUM; ++i) {
            genomes.array[i].score = board.scores[i];
        }
    }

    inline void calculateScoreAndReplace(const int& id, FullGenome g) {// Not sure about putting a ref here or not
        calculateScore(id, g, *global_board);
        for(char i = 0;i<GLOBAL_PLAYER_NUM;++i){
            this->theTopGenomes[i].addSup(g.array[i]);
        }
        ++global_compute;
    }

    inline void evolveOnce(const int & id) {
        ++global_generation;
        uint i = 0;
        //insert previous generation best
        for (; i< GLOBAL_TOP_PERCENT; ++i) {
            this->theFullGenomes[i] = FullGenome(this->theTopGenomes[0].array[i],this->theTopGenomes[1].array[i],this->theTopGenomes[2].array[i],this->theTopGenomes[3].array[i]);
        }
        //May be add pure random gene
		for (; i< GLOBAL_POPULATION_SIZE/2; ++i) {
		    this->theFullGenomes[i] = FullGenome();
        }
        //cross breed the remaining from best
        for (; i < GLOBAL_POPULATION_SIZE; ++i) {
            int index_genome1 = int ((static_cast <float> (rand()) / static_cast <float> (RAND_MAX)) * (GLOBAL_TOP_PERCENT)) ;
            int index_genome2 = int ((static_cast <float> (rand()) / static_cast <float> (RAND_MAX)) * (GLOBAL_TOP_PERCENT)) ;
            // We have a new genome with a new score
            this->theFullGenomes[i].cross(this->theFullGenomes[index_genome1],this->theFullGenomes[index_genome2]);
        }
        //actual process new scores
        if(global_generation == 1){
            i = 0; //need to recalculate full as they were no done with rigth ennemies
        }else{
            i = 10;
        }
        for (; i < GLOBAL_POPULATION_SIZE && !(global_timer->isTimesUp()); ++i) {
            this->calculateScoreAndReplace(id, this->theFullGenomes[i]);
        }
    }

    inline void evolve(const int& id) {
        while(!(global_timer->isTimesUp())){
            this->evolveOnce(id);
        }
    }

    inline FullGenome findBestFullGenome(const int& id) {
        FullGenome aFullgenome = FullGenome();
        long int best_score=LONG_MIN;
        char best = -1;
        for(char i=0; i < GLOBAL_TOP_PERCENT;++i){
            //cerr << "best gene" << theTopGenomes[id].array[i].array[0].toString() <<endl;
            long int temp_score=0;
            aFullgenome = FullGenome(theTopGenomes[0].array[i],theTopGenomes[1].array[i],theTopGenomes[2].array[i],theTopGenomes[3].array[i]);
            for(char j=0; j < GLOBAL_TOP_PERCENT;++j){
                for(char k=0; k < GLOBAL_PLAYER_NUM;++k){
                    if(k!=id){
                        aFullgenome.update(k,theTopGenomes[k].array[j]);
                    }
                }
                calculateScore(id, aFullgenome, *global_board);
                temp_score += aFullgenome.array[id].score;
            }
            if(temp_score > best_score){
                best_score = temp_score;
                best = i;
            }
        }
        cerr << "best_score" << best_score <<endl;
        return FullGenome(theTopGenomes[0].array[best],theTopGenomes[1].array[best],theTopGenomes[2].array[best],theTopGenomes[3].array[best]);
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
    res += b.getNextWithoutCheck(g, b.players[id].p).toString();
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
    int myId;
    cin >> width >> height >> myId; cin.ignore();
    Board theBoard = Board();
    global_board = &theBoard;
    Board previous_board;

    // game loop
    FullGenome bestFullGenomes;
    while (1)
    {
        global_debug=false;
        global_compute = 0;
        global_generation = 0;
        previous_board = theBoard;
        myQueue<Square*> deleteBox;
        global_board->bigBadaboum(deleteBox);
        for (int i = 0; i < height; i++)
        {
            string row;
            getline(cin, row);
            global_board->init(i,row);
        }
        int entities;
        cin >> entities; cin.ignore();
        global_board->bombs.clear();
        for (int i = 0; i < GLOBAL_PLAYER_NUM; i++) {
            global_board->scores[i]=0;
        }
        for (int i = 0; i < entities; i++) {
            int entityType;
            int owner;
            int x;
            int y;
            int param1;
            int param2;
            cin >> entityType >> owner >> x >> y >> param1 >> param2; cin.ignore();
            global_board->init(entityType, owner, x, y, param1, param2, previous_board);
        }
        // for(int i =0 ;i<4;++i )
        //     cerr << global_board->players[i].toString() << endl;
        global_debug=false;
        Timer timer = Timer(first_turn);
        global_timer = &timer;
        //global_board->toString();
        global_debug=false;
        bestFullGenomes.nextGen();
        Evolution evol(myId, GLOBAL_INITIAL_POP_SIZE, bestFullGenomes );
        evol.evolve(myId);
        bestFullGenomes = evol.findBestFullGenome(myId);
        // for(int i =0;i<4;++i){
        //     bestFullGenomes.update(i, evol.theTopGenomes[i].top());
        // }
        const Genome& myBestGenome = bestFullGenomes.array[myId];
        global_debug=false;
        //cerr << "PLayer" << global_board->players[global_board->myId].toString() << endl;
        // Board tmp_board = *global_board;
        // cerr << "BEFORE" << endl;
        // tmp_board.toString();
        // Gene gArray[GLOBAL_PLAYER_NUM];
        // bestFullGenomes.genes(0, gArray);
        // tmp_board.update(gArray, 16);
        // cerr << "AFTER" << endl;
        // tmp_board.toString();
        //evol.calculateScore(myId, bestFullGenomes, tmp_board);
        global_debug=false;
        // for(int i =0;i<4;++i){
        //     cerr << "Player: " << i << " Score: " << bestFullGenomes.array[i].score << endl;
        //     cerr << bestFullGenomes.array[i].toString() << endl;
        // }
        // for(int i =0;i<10;++i){
        //     cerr << " Score: " << evol.theTopGenomes[myId].array[i].score << endl;
        // }
        // Write an action using cout. DON'T FORGET THE "<< endl"
        // To debug: cerr << "Debug messages..." << endl;

        cout << output(myId, myBestGenome.array[0], *global_board) << endl;
        score_cumul += global_compute;
        cerr << "average:" << (score_cumul/global_turn) << endl;
        ++global_turn;
    }
}