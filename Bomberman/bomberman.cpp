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
#include <queue>

using namespace std;

const signed char GLOBAL_TURN_TIME = 100;
const signed char GLOBAL_TURN_TIME_MAX = 90;
const int GLOBAL_TURN_TIME_MAX_FIRST_TURN = 450;
const signed char GLOBAL_GENOME_SIZE = 8;
const uint GLOBAL_GENOME_SAMPLE_SIZE = 50000;
const signed char GLOBAL_MAX_WIDTH = 13;
const signed char GLOBAL_MAX_HEIGHT = 11;
bool global_debug = false;
const signed char GLOBAL_PLAYER_NUM = 4;

struct Board;
Board* global_board;
struct Player;
Player* global_my_player;

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
    inline string toString()
    {
        return std::to_string(this->x) + " " + std::to_string(this->y);
    }
    inline string toString(string debug)
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
};

struct Player {
    char id;
    char range;
    char cur_stock;
    char max_stock;
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
        this->range = 2;
        this->cur_stock = 1;
        this->max_stock = 1;
        this->p = p;
        this->isAlive = true;
    }

    inline void update(const char& owner_id, const Point & p){
        this->id = owner_id;
        this->p = p;
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
};

struct Bomb {
    char owner;
    char range;
    char timer;
    Point p;


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
};

struct Square {
    enum type { empty, box, box_b_range, box_b_stock, item_b_range,item_b_stock, wall};
    type t;
    Point p;
    bool hasPlayer;
    bool hasBomb;

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
        this->hasBomb=true;
    }
    inline void addBox(){
        this->t=type::box;
    }
    inline void addWall(){
        this->t=type::wall;
    }
    inline void setEmpty(){
        this->t=type::empty;
        this->hasBomb = false;
        this->hasPlayer = false;
    }
    inline bool canEnter() const {
        if (this->t == type::empty ||
            this->t == type::item_b_range ||
            this->t == type::item_b_stock) {
            return true;
        }
        return false;
    }
    inline bool isBox() const {
        if (this->t == type::box ||
            this->t == type::box_b_range ||
            this->t == type::box_b_stock) {
            return true;
        }
        return false;
    }
    inline bool hasBonus() const {
        if (this->t == type::box_b_range  ||
            this->t == type::box_b_stock  ||
            this->t == type::item_b_range ||
            this->t == type::item_b_stock ) {
            return true;
        }
        return false;
    }
    inline void addPlayer() {
        this->hasPlayer=true;
    }
    inline void removePlayer() {
        this->hasPlayer=false;
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
        return this->hasBomb;
    }
    inline bool blocksExplosion() const {
        if (this->t == type::wall ||
            this->hasBomb ||
            this->t == type::box ||
            this->t == type::box_b_range ||
            this->t == type::box_b_stock ||
            this->t == type::item_b_range ||
            this->t == type::item_b_stock) {
            return true;
        }
        return false;
    }
    inline bool canBeDestroyed() const {
        if (this->hasBomb ||
            this->t == type::box ||
            this->t == type::box_b_range ||
            this->t == type::box_b_stock ||
            this->t == type::item_b_range ||
            this->t == type::item_b_stock) {
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
        this->hasBomb = false;
        this->hasPlayer = false;
    }
    inline string toString() {
        return "location " + this->p.toString() + " type " + to_string(this->t) + " hasPlayer " + to_string(this->hasPlayer) + " hasBomb " + to_string(this->hasBomb);
    }
};

struct Gene {
    float move;
    bool bomb;

    inline Gene(Gene const&) = default;
    inline Gene(Gene&&) = default;
    inline Gene& operator=(Gene const&) = default;
    inline Gene& operator=(Gene&&) = default;

    inline Gene () : move(0), bomb(false) {}
    inline Gene(float m, bool b) : move(m), bomb(b) {}

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


struct Board
{
    char myId;
    char height;
    char width;
    Square theBoard[GLOBAL_MAX_WIDTH][GLOBAL_MAX_HEIGHT];
    Player players [GLOBAL_PLAYER_NUM];
    Player* myPlayer = NULL;
    list<Bomb> bombs;
    int score;

    inline Board() = default;
    inline Board(Board const&) = default;
    inline Board(Board&&) = default;
    inline Board& operator=(Board const&) = default;
    inline Board& operator=(Board&&) = default;

    inline Board(char iMyID, char w, char h){
        this->myId = iMyID;
        this->height=h;
        this->width=w;
        this->score = 0;
         for(char y= 0; y< this->height;++y){
            for(char x= 0; x< this->width;++x){
               this->theBoard[x][y]=Square(Point(x, y));
            }
        }
        for(char i= 0; i< GLOBAL_PLAYER_NUM;++i){
           this->players[i] = Player(i,Point());
        }
        this->myPlayer = &(this->players[this->myId]);
    }
    inline Square get(int x, int y) {
        return this->theBoard[x][y];
    }
    inline void init(int i, string row)
    {
        for(char x =0;x<this->width;++x)
        {
            if (row[x] == '.') {
                this->theBoard[x][i].setEmpty();
            }else if (row[x] == 'X') {
                this->theBoard[x][i].addWall();
            } else {
                this->theBoard[x][i].addBox();
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
        if (this->score >= 0) {
            // Only increase if we are not dead
            this->score += n;
        }
    }
    inline void killPlayersOnSquare(const Point& p) {
        for (char i=0; i<GLOBAL_PLAYER_NUM ; ++i) {
            if (this->players[i].p.x == p.x && this->players[i].p.y == p.y) {
                // Then player is dead
                this->players[i].kill();
                if (i == this->myId) {
                    // My player is dead, game over
                    this->score = -1;
                } else {
                    // Another player is dead, this is good news !
                    this->score += 25;
                }
            }
        }
    }
    inline void addBombToExplosionList(const Point & p, queue<Bomb*> &explosionList){
        // Add bombs in point that have timer > 0
        for (list<Bomb>::iterator it = this->bombs.begin(); it != this->bombs.end(); ++it) {
            if (p.x == it->p.x && p.y == it->p.y && it->timer > 0) {
                it->timer = 0;
                explosionList.push(&(*it));
            }
        }
    }
    inline bool processBomb(const Bomb & aBomb, queue<Bomb*> &explosionList, queue<Square*> &deletedObjects) {
        // Right
        for (char x=0; x < aBomb.range && aBomb.p.x+x < this->width; ++x) {
            if (this->theBoard[aBomb.p.x+x][aBomb.p.y].hasPlayer ||
                this->theBoard[aBomb.p.x+x][aBomb.p.y].canBeDestroyed()) {
                deletedObjects.push(&(this->theBoard[aBomb.p.x+x][aBomb.p.y]));
            }
            if (this->theBoard[aBomb.p.x+x][aBomb.p.y].blocksExplosion()) {
                if (this->theBoard[aBomb.p.x+x][aBomb.p.y].containsBomb()){
                    this->addBombToExplosionList(aBomb.p, explosionList);
                }
                if (this->theBoard[aBomb.p.x+x][aBomb.p.y].isBox()) {
                    // Give point to player
                    players[aBomb.owner].increaseScore();
                }
                break;
            }
        }
        // Left
        for (char x=0; x < aBomb.range && aBomb.p.x-x >= 0; ++x) {
            if (this->theBoard[aBomb.p.x-x][aBomb.p.y].hasPlayer ||
                this->theBoard[aBomb.p.x-x][aBomb.p.y].canBeDestroyed()) {
                deletedObjects.push(&(this->theBoard[aBomb.p.x-x][aBomb.p.y]));
            }
            if (this->theBoard[aBomb.p.x-x][aBomb.p.y].blocksExplosion()) {
                if (this->theBoard[aBomb.p.x-x][aBomb.p.y].containsBomb()){
                    this->addBombToExplosionList(aBomb.p, explosionList);
                }
                if (this->theBoard[aBomb.p.x-x][aBomb.p.y].isBox()) {
                    // Give point to player
                    players[aBomb.owner].increaseScore();
                }
                break;
            }
        }
        // Down
        for (char y=0; y < aBomb.range && aBomb.p.y+y < this->height; ++y) {
            if (this->theBoard[aBomb.p.x][aBomb.p.y+y].hasPlayer ||
                this->theBoard[aBomb.p.x][aBomb.p.y+y].canBeDestroyed()) {
                deletedObjects.push(&(this->theBoard[aBomb.p.x][aBomb.p.y+y]));
            }
            if (this->theBoard[aBomb.p.x][aBomb.p.y+y].blocksExplosion()) {
                if (this->theBoard[aBomb.p.x][aBomb.p.y+y].containsBomb()){
                    this->addBombToExplosionList(aBomb.p, explosionList);
                }
                if (this->theBoard[aBomb.p.x][aBomb.p.y+y].isBox()) {
                    // Give point to player
                    players[aBomb.owner].increaseScore();
                }
                break;
            }
        }
        // Up
        for (char y=0; y < aBomb.range && aBomb.p.y-y >= 0; ++y) {
            if (this->theBoard[aBomb.p.x][aBomb.p.y-y].hasPlayer ||
                this->theBoard[aBomb.p.x][aBomb.p.y-y].canBeDestroyed()) {
                deletedObjects.push(&(this->theBoard[aBomb.p.x][aBomb.p.y+y]));
            }
            if (this->theBoard[aBomb.p.x][aBomb.p.y-y].blocksExplosion()) {
                if (this->theBoard[aBomb.p.x][aBomb.p.y-y].containsBomb()){
                    this->addBombToExplosionList(aBomb.p, explosionList);
                }
                if (this->theBoard[aBomb.p.x][aBomb.p.y-y].isBox()) {
                    // Give point to player
                    players[aBomb.owner].increaseScore();
                }
                break;
            }
        }
        return false; // Default we suppose we are safe
    }
    inline void bigBadaboum() {
        // Go decrement all bomb timers
        queue<Bomb*> explosionList;
        queue<Square*> deletedObjects;
        for(list<Bomb>::iterator it = this->bombs.begin(); it != this->bombs.end();++it){
            it->tick();
            if (it->isExploding()) {
                explosionList.push(&(*it));
            }
        }
        // Simultaneous explosions
        while(!explosionList.empty()){
            Bomb* pBomb = explosionList.front(); // Access 1st elem
            processBomb(*pBomb,explosionList, deletedObjects);
            explosionList.pop(); // Delete 1st elem
        }
        // Cleaning the map
        while(!deletedObjects.empty()){
            Square* pSquare = deletedObjects.front();
            pSquare->explose();
            if (pSquare->hasPlayer) {
                this->killPlayersOnSquare(pSquare->p);
            }
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
    inline uint boxInRange(const Bomb& bomb){
        uint res = 0 ;
        for (char x = 0; x < bomb.range && bomb.p.x+x <=this->width; ++x){
            if (global_debug) cerr << "Testing Case : " << this->theBoard[bomb.p.x+x][bomb.p.y].p.toString() << endl;
            if (global_debug) cerr << "Type Case : " << this->theBoard[bomb.p.x+x][bomb.p.y].t << endl;
            if (this->theBoard[bomb.p.x+x][bomb.p.y].t == Square::type::box){
                if (global_debug) cerr << "Box RIGHT: " << this->theBoard[bomb.p.x+x][bomb.p.y].p.toString() << endl;
                ++res;
                break;
            } else if (this->theBoard[bomb.p.x+x][bomb.p.y].t == Square::type::wall) {
                break;// A wall blocks the explosion, no need to look further
            }
        }


        for (char x = 0; x > -bomb.range && bomb.p.x+x >= 0; --x) {
            if (this->theBoard[bomb.p.x+x][bomb.p.y].t == Square::type::box){
                  if (global_debug) cerr << "Box LEFT: " << this->theBoard[bomb.p.x+x][bomb.p.y].p.toString() << endl;
                ++res;
                break;
            } else if (this->theBoard[bomb.p.x+x][bomb.p.y].t == Square::type::wall) {
                break;// A wall blocks the explosion, no need to look further
            }
        }

        for (char y = 0; y < bomb.range && bomb.p.y+y <= this->height; ++y){
            if (this->theBoard[bomb.p.x][bomb.p.y+y].t == Square::type::box){
                if (global_debug) cerr << "Box DOWN: " << this->theBoard[bomb.p.x][bomb.p.y+y].p.toString() << endl;
                ++res;
                break;
            } else if (this->theBoard[bomb.p.x][bomb.p.y+y].t == Square::type::wall) {
                break;// A wall blocks the explosion, no need to look further
            }
        }

        for (char y = 0; y > -bomb.range && bomb.p.y+y >= 0; --y){
            if (this->theBoard[bomb.p.x][bomb.p.y+y].t == Square::type::box){
                if (global_debug) cerr << "Box UP: " << this->theBoard[bomb.p.x][bomb.p.y+y].p.toString() << endl;
                ++res;
                break;
            } else if (this->theBoard[bomb.p.x][bomb.p.y+y].t == Square::type::wall) {
                break;// A wall blocks the explosion, no need to look further
            }
        }
        return res;
    }


    inline void update(const Gene& g, int multiplier){

        // cf. Experts rules for details
        // First: bombs explodes (if reach timer 0) and destroy objects
        this->bigBadaboum();

        // Treat the bomb dropped case TODO include in bigBadaboum
        if (g.bomb) {
            // Add bomb on the sqaure and in the list of bombs too
            Bomb aBomb(this->myId, this->myPlayer->range, 8 /* Timer 8 for all new bombs */, this->myPlayer->p);
            this->theBoard[this->myPlayer->p.x][this->myPlayer->p.y].addBomb();
            // TODO : change the behavior for the actual bomd explosion instead of anticipating
            this->increaseScore(this->boxInRange(aBomb) * multiplier);

            if (global_debug) cerr << "Case is: " << this->theBoard[this->myPlayer->p.x][this->myPlayer->p.y].toString() << endl;
            if (global_debug) cerr << "Player is: " << this->players[myId].p.toString() << endl;
            if (global_debug) {
                cerr << "Bomb will be here " << this->myPlayer->p.toString() << endl;
                cerr << "boxInRange to " << this->boxInRange(aBomb) << " boxes with multiplier " << multiplier << endl;
                cerr << "Setting bomb to " << this->myPlayer->p.toString() << endl;
            }
        }

        // Then: we move the player(s)
        Point new_pos = this->getNext(g, this->myPlayer->p);
        // Treat the movement of the player
        if ((this->theBoard[new_pos.x][new_pos.y].canEnter()) &&
           new_pos.x >= 0 && new_pos.x < this->width &&
           new_pos.y >= 0 && new_pos.y < this->height) { // valid move
            if (this->theBoard[new_pos.x][new_pos.y].hasBonus()) { // we take an item
               this->increaseScore(2*multiplier);
            }
            // update the new square with the player information
            this->theBoard[new_pos.x][new_pos.y].addPlayer();
            this->theBoard[this->myPlayer->p.x][this->myPlayer->p.y].removePlayer();
        }
        if (global_debug) {cerr << "Player moved here " << this->players[myId].p.toString() << endl;}

        // New bombs finally appears
        if (g.bomb) {
            // Currently called twice, but should decrease to only this one after refactoring the above TODO
            this->theBoard[this->players[myId].p.x][this->players[myId].p.y].addBomb();
        }
    }


    inline void toString(){
        for(char y= 0; y< this->height;++y){
            for(char x= 0; x< this->width;++x){
                cerr << this->theBoard[x][y].t <<" ";
            }
            cerr << endl;
        }
    }
};

struct Genome {
    Gene array[GLOBAL_GENOME_SIZE];
    inline Genome() = default;
    inline Genome(Genome const&) = default;
    inline Genome(Genome&&) = default;
    inline Genome& operator=(Genome const&) = default;
    inline Genome& operator=(Genome&&) = default;
    inline Genome (char bomb_t, char bomb_stock) {
        for(char i = 0;i<GLOBAL_GENOME_SIZE;++i){
            if(bomb_t>0){
                this->array[i]=Gene(static_cast <float> (rand()) / static_cast <float> (RAND_MAX),false);
                --bomb_t;
            }else{
                if(static_cast <float> (rand()) / static_cast <float> (RAND_MAX) > 0.50){
                    this->array[i]=Gene(static_cast <float> (rand()) / static_cast <float> (RAND_MAX),true);
                    bomb_t=8;
                }else{
                    this->array[i]=Gene(static_cast <float> (rand()) / static_cast <float> (RAND_MAX),false);
                }
            }
        }
    }
    inline string toString(){
        string res = "";
        for (int i =0;i<GLOBAL_GENOME_SIZE;++i){
            res += this->array[i].toString() + "\n";
        }
        return res;
    }
};



uint calculateScore(const Genome& genome, Board board)
{
    for (char i=0; i<GLOBAL_GENOME_SIZE; ++i) {
        if (global_debug) {cerr << "We update with gene: " << genome.array[i].toString() << endl;}
        board.update(genome.array[i], GLOBAL_GENOME_SIZE-i);
        if (global_debug) {cerr << "New score: " << board.score << endl;}
    }
    return board.score;
}

string output(const Gene& g, const Board& b){
    string res = "";
    if (g.bomb) {
        res += "BOMB";
    } else {
        res += "MOVE";
    }
    res += " ";
    res += b.getNext(g, b.myPlayer->p).toString();
    return res;
}

/**
 * Auto-generated code below aims at helping you parse
 * the standard input according to the problem statement.
 **/
int main()
{
    Timer  * timer;
    bool first_turn = false;
    int width;
    int height;
    int myId;
    cin >> width >> height >> myId; cin.ignore();
    Board theBoard(myId, width, height);
    global_board = &theBoard;
    char player_num=2;
    Square* players[player_num];
    // TODO implement a real bomb stock manager
    char next_bomb = 0;
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
        global_board->toString();
        global_debug=false;

        Genome someGenomes[GLOBAL_GENOME_SAMPLE_SIZE];
        uint bestGenome=0;
        uint bestScore=0;
        for (uint i=0; i<GLOBAL_GENOME_SAMPLE_SIZE; ++i) {
            uint newScore=0;
            someGenomes[i] = Genome(next_bomb, global_board->myPlayer->cur_stock);
            newScore = calculateScore(someGenomes[i], *global_board);
            if (newScore > bestScore) {
                bestScore = newScore;
                bestGenome = i;
            }
            if (timer.isTimesUp()) {
                cerr << "We calculated " << i << " genomes before time out" << endl;
                break;
            }
        }
        global_debug=true;
        //Genome myGenome(0);
        //myGenome.array[0] = Gene(0, true);
        //myGenome.array[1] = Gene(0.5, true);
        calculateScore(someGenomes[bestGenome], *global_board);
        global_debug=false;
        cerr << "Best score selected is " << bestScore << endl;
        cerr << someGenomes[bestGenome].toString() << endl;
        // Write an action using cout. DON'T FORGET THE "<< endl"
        // To debug: cerr << "Debug messages..." << endl;
        global_board->toString();
        cout << output(someGenomes[bestGenome].array[0], *global_board) << endl;
        if (someGenomes[bestGenome].array[0].bomb) {
            next_bomb = 8;
        }
        --next_bomb;
    }
}