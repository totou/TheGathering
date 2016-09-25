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
const signed char GLOBAL_GENOME_SIZE = 16;
const uint GLOBAL_GENOME_SAMPLE_SIZE = 50000;
const signed char GLOBAL_MAX_WIDTH = 13;
const signed char GLOBAL_MAX_HEIGHT = 11;
bool global_debug = false;

struct Board;
Board* global_board;
struct Player;
Player* global_my_player;

struct Timer{
    chrono::time_point<chrono::system_clock> end;
    signed char not_always = 0;
    Timer(bool first_turn=false){
        if (first_turn) {
            this->end=chrono::system_clock::now()+chrono::milliseconds(GLOBAL_TURN_TIME_MAX_FIRST_TURN);
        } else {
            this->end=chrono::system_clock::now()+chrono::milliseconds(GLOBAL_TURN_TIME_MAX);
        }
    }
    bool isTimesUp(){
        return std::chrono::system_clock::now() > this->end;
    }
};

struct Point
{
    char x;
    char y;
    Point() {
        this->x = 0;
        this->y = 0;
    }
    Point(const Point& p) {
        this->x = p.x;
        this->y = p.y;
    }
    Point(char x1,char y1) : x(x1), y(y1) {
    }
    string toString()
    {
        return std::to_string(this->x) + " " + std::to_string(this->y);
    }
    string toString(string debug)
    {
        return this->toString() + " " + debug;
    }
    void correctBounds() {
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

struct Square {
    enum type { player, empty, box, bomb, item};
    type t;
    Point p;
    char timer;
    char range;
    Square() {
        this->p = Point(-1,-1);
        this->t = type::empty;
        this->timer=-1;
        this->range=-1;
    }
    void update(Square::type t, Point p, char timer, char range) {
        this->p=p;
        this->t=t;
        this->timer=timer;
        this->range=range;
    }
    void update(Square s) {//take the attributs of another square
        this->t=s.t;
        this->timer=s.timer;
        this->range=s.range;
    }
    void setBomb(){
        this->t=type::bomb;
        this->timer=8;
        this->range=2;
    }
    void setEmpty(){
        this->t=type::empty;
        this->timer=-1;
        this->range=-1;
    }

    string toString() {
        return "location " + this->p.toString() + " type " + to_string(this->t) + " timer " + to_string(this->timer) + " range " + to_string(this->range);
    }
};

struct Gene {
    float move;
    bool bomb;
    Gene () : move(0), bomb(false) {}
    Gene(float m, bool b) : move(m), bomb(b) {}

    Point getNext(const Point& p) const
    {
        Point pres(p);
        if (this->move <0.2) {
            return p;
        } else if (this->move >=0.2 && this->move < 0.4) {
            pres.x += 1;
        } else if (this->move >= 0.4 && this->move < 0.6) {
            pres.y += 1;
        } else if (this->move >= 0.6 && this->move < 0.8) {
            pres.x -= 1;
        } else if (this->move >= 0.8) {
            pres.y -= 1;
        }
        pres.correctBounds();
        return pres;
    }
    string toString() const {
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
    char height;
    char width;
    Square theBoard[GLOBAL_MAX_WIDTH][GLOBAL_MAX_HEIGHT];
    Square* player;
    uint score;

    Board(char w, char h){
        this->height=h;
        this->width=w;
        this->player == NULL;
        this->score = 0;
         for(char y= 0; y< this->height;++y){
            for(char x= 0; x< this->width;++x){
               this->theBoard[x][y]=Square();
            }
        }
    }
    Square get(int x, int y) {
        return this->theBoard[x][y];
    }
    void init(int i, string row)
    {
        for(char x =0;x<this->width;++x)
        {
            if(row[x] == '.'){
                this->theBoard[x][i].update(Square::empty,Point(x,i),-1,-1);
            }else{
                this->theBoard[x][i].update(Square::box,Point(x,i),-1,-1);
            }
        }
    }
    void init(int myId, int entityType, int owner, int x, int y, int param1, int param2)
    {
        if (entityType == 0 )//player
        {
            this->theBoard[x][y].update(Square::player,Point(x,y),-1,-1);
            if(myId == owner) {
                this->player = &(this->theBoard[x][y]);
            }
        }else if (entityType == 1){ //Bomb
            this->theBoard[x][y].update(Square::bomb,Point(x,y), param1, param2);
        }else if (entityType == 2){ // Item
            this->theBoard[x][y].update(Square::item,Point(x,y), param1,-1);
        }
    }
    uint boxInRange(const Square& bomb){
        uint res = 0 ;
        if (global_debug) cerr << "Range : " << to_string(bomb.range) << endl;
        if (global_debug) cerr << "Width : " << this->width << endl;
        if (global_debug) cerr << "Valid : " << to_string(bomb.p.x+2) << endl;
        if (global_debug) cerr << "Valid2 : " << (bomb.p.x+2 <= this->width) << endl;
        for (char x = 0; x <= bomb.range && bomb.p.x+x <=this->width; ++x){
            if (global_debug) cerr << "Testing Case : " << this->theBoard[bomb.p.x+x][bomb.p.y].p.toString() << endl;
            if (global_debug) cerr << "Type Case : " << this->theBoard[bomb.p.x+x][bomb.p.y].t << endl;
            if (this->theBoard[bomb.p.x+x][bomb.p.y].t == Square::type::box){
                if (global_debug) cerr << "Box RIGHT: " << this->theBoard[bomb.p.x+x][bomb.p.y].p.toString() << endl;
                ++res;
                break;
            }
        }


        for (char x = 0; x >= -bomb.range && bomb.p.x+x >= 0; --x) {
            if (this->theBoard[bomb.p.x+x][bomb.p.y].t == Square::type::box){
                  if (global_debug) cerr << "Box LEFT: " << this->theBoard[bomb.p.x+x][bomb.p.y].p.toString() << endl;
                ++res;
                break;
            }
        }

        for (char y = 0; y <= bomb.range && bomb.p.y+y <= this->height; ++y){
            if (this->theBoard[bomb.p.x][bomb.p.y+y].t == Square::type::box){
                if (global_debug) cerr << "Box DOWN: " << this->theBoard[bomb.p.x][bomb.p.y+y].p.toString() << endl;
                ++res;
                break;
            }
        }

        for (char y = 0; y >= -bomb.range && bomb.p.y+y >= 0; --y){
            if (this->theBoard[bomb.p.x][bomb.p.y+y].t == Square::type::box){
                if (global_debug) cerr << "Box UP: " << this->theBoard[bomb.p.x][bomb.p.y+y].p.toString() << endl;
                ++res;
                break;
            }
        }

        return res;
    }


    void update(const Gene& g, int multiplier){
        Point new_pos = g.getNext(this->player->p);

        // Treat the bomb case
        if (g.bomb) {
            this->theBoard[this->player->p.x][this->player->p.y].setBomb();
            // TODO : change the behavior for the actual bomd explosion instead of anticipating
            this->score += this->boxInRange(this->theBoard[this->player->p.x][this->player->p.y]) * multiplier;
            
            if (global_debug) cerr << "Case is: " << this->theBoard[this->player->p.x][this->player->p.y].toString() << endl;
            if (global_debug) cerr << "Player is: " << (*(this->player)).p.toString() << endl;
            if (global_debug) {
                cerr << "Bomb will be here " << this->player->p.toString() << endl;
                cerr << "boxInRange to " << this->boxInRange(*(this->player)) << " boxes with multiplier " << multiplier << endl;
                cerr << "Setting bomb to " << this->player->p.toString() << endl;
            }
        }

        // Treat the movement of the player
        if (this->theBoard[new_pos.x][new_pos.y].t == Square::type::empty &&
           new_pos.x >= 0 && new_pos.x < this->width &&
           new_pos.y >= 0 && new_pos.y < this->height) { // valid move
            // update the new square with the player information
            this->theBoard[new_pos.x][new_pos.y].update(this->theBoard[this->player->p.x][this->player->p.x]);
            this->theBoard[this->player->p.x][this->player->p.y].setEmpty();
            // update the new pointer to the new player square
            this->player = &(this->theBoard[new_pos.x][new_pos.y]);
        }

        if (global_debug) {cerr << "Player moved here " << this->player->p.toString() << endl;}
    }


    void toString(){
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
    Genome () {}
    Genome (char bomb_t) {
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
    string toString(){
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
    res += g.getNext(b.player->p).toString();
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
    Board theBoard(width, height);
    global_board = &theBoard;
    char player_num=2;
    Square* players[player_num];
    // TODO implement a real bomb stock manager
    char next_bomb = 0;
    // game loop
    while (1)
    {
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
            global_board->init(myId, entityType, owner, x, y, param1, param2);
        }
        Timer timer = Timer(first_turn);
        global_board->toString();

        Genome someGenomes[GLOBAL_GENOME_SAMPLE_SIZE];
        uint bestGenome=0;
        uint bestScore=0;
        for (uint i=0; i<GLOBAL_GENOME_SAMPLE_SIZE; ++i) {
            uint newScore=0;
            someGenomes[i] = Genome(next_bomb);
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
        //calculateScore(myGenome, *global_board);
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