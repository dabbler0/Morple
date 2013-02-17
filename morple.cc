#include <iostream>
#include <cmath>
#include <sstream>
#include <fstream>
using namespace std;

/*
Morple

A basic rock-paper-scissors AI.

Copyright (c) 2013 Anthony Bau.
*/

bool DEBUG = false;
ofstream DEBUG_FILE ("morple.debug");

class Prediction {
  private:
    double p[3];
    double t;
  public:
    Prediction () {
      p[0] = p[1] = p[2] = t = 0;
    }
    Prediction (double rock, double paper, double scissors) {
      p[0] = rock;
      p[1] = paper;
      p[2] = scissors;
      t = rock + paper + scissors;
    }

    void shift(int n) {
      if (n % 3 == 0) return;
      double temp[3];
      for (int i = 0; i < 3; i += 1) temp[i] = p[i];
      for (int i = 0; i < 3; i += 1) p[i] = temp[(n + i) % 3];
    }

    Prediction operator+ (Prediction o) {
      Prediction r (p[0] + o.p[0], p[1] + o.p[1], p[2] + o.p[2]);
      return r;
    }

    void operator+= (Prediction o) {
      for (int i = 0; i < 3; i += 1) p[i] += o.p[i];
      t += o.t;
    }

    void operator= (Prediction o) {
      for (int i = 0; i < 3; i += 1) p[i] = o.p[i];
      t = o.t;
    }
    
    Prediction scaleTo (double n) {
      if (t == 0) {
        Prediction r (n/3,n/3,n/3);
        return r;
      }
      else {
        Prediction r (p[0]*n/t,p[1]*n/t,p[2]*n/t);
        return r;
      }
    }

    double operator[] (int n) {
      if (t == 0) {
        return 1.0/3.0;
      }
      else {
        return ((double) p[n]) / t;
      }
    }
    
    double* as_array() {
      double* r = new double[3];
      if (t > 0) for (int i = 0; i < 3; i += 1) r[i] = (double) p[i] / t;
      else for (int i = 0; i < 3; i += 1) r[i] = 1.0/3.0;
      return r;
    }

    int generate() {
      if (p[2] - p[1] > p[0] - p[2] && p[2] - p[1] > p[1] - p[0]) return 0;
      else if (p[0] - p[2] > p[1] - p[0]) return 1;
      else return 2;
    }
};

class Predictor {
  public:
    int mod;
    Predictor () {
      mod = 0;
    }
    void shift(int n) {
      mod += n;
    }
    virtual void feed(int,int) = 0;
    virtual Prediction raw_predict() = 0;
    Prediction predict() {
      Prediction r = raw_predict();
      r.shift(mod);
      return r;
    }
};

//Picks according to how often moves are thrown
class UnigramCounter : public Predictor {
  private:
    double counts[3];
  public:
    UnigramCounter() {
      counts[0] = 0;
      counts[1] = 0;
      counts[2] = 0;
    }
    void feed(int n, int k) {
      for (int i = 0; i < 3; i += 1) counts[i] *= 0.96;
      counts[n % 3] += 1;
    }
    Prediction raw_predict() {
      if (DEBUG) DEBUG_FILE << "Unigrams has " << counts[0] << ',' << counts[1] << ',' << counts[2] << '(' << mod << ')' << endl;
      Prediction r (counts[0], counts[1], counts[2]);
      return r;
    }
};

//Picks according to a Markov model on opponents' throws
class MarkovCounter : public Predictor {
  private:
    double counts[3][3];
    int last;
  public:
    MarkovCounter() {
      for (int i = 0; i < 3; i += 1) for (int x = 0; x < 3; x += 1) counts[i][x] = 0; //Fill our count array with zeroes
      last = -1;
    }
    void feed(int n, int k) {
      for (int i = 0; i < 3; i += 1) for (int x = 0; x < 3; x += 1) counts[i][x] *= 0.95; //We slowly forget about past events
      if (last >= 0) counts[last][n] += 1; //Add one to the proper count
      last = n; //Update last
    }
    Prediction raw_predict() {
      if (DEBUG) DEBUG_FILE << "For " << last << ", Markov has " << counts[last][0] << "," << counts[last][1] << "," << counts[last][2] << '(' << mod << ')' << endl;
      Prediction r (counts[last][0], counts[last][1], counts[last][2]);
      return r;
    }
};

//Picks according to a Markov model on our own throws
class SelfMarkovCounter : public Predictor {
  private:
    double counts[3][3];
    int last;
  public:
    SelfMarkovCounter() {
      for (int i = 0; i < 3; i += 1) for (int x = 0; x < 3; x += 1) counts[i][x] = 0; //Fill our count array with zeroes
      last = -1;
    }
    void feed (int n, int k) {
      for (int i = 0; i < 3; i += 1) for (int x = 0; x < 3; x += 1) counts[i][x] *= 0.95; //We slowly forget about past events
      if (last >= 0) counts[last][n] += 1; //Add one to the proper count
      last = k; //Update last
    }
    Prediction raw_predict() {
      if (DEBUG) DEBUG_FILE << "For " << last << ", Self-Markov has " << counts[last][0] << ',' << counts[last][1] << ',' << counts[last][2] << '(' << mod << ')';
      Prediction r (counts[last][0], counts[last][1], counts[last][2]);
      return r;
    }
};

//Picks according to a Markov model on tuples of throws (mine,theirs)
class DualMarkovCounter : public Predictor {
  private:
    double counts[9][3];
    int last;
  public:
    DualMarkovCounter() {
      for (int i = 0; i < 9; i += 1) for (int x = 0; x < 3; x += 1) counts[i][x] = 0; //Fill our count array with zeroes
      last = -1;
    }
    
    void feed(int n, int k) {
      for (int i = 0; i < 9; i += 1) for (int x = 0; x < 3; x += 1) counts[i][x] *= 0.96; //We slowly forget about past events
      if (last >= 0) counts[last][n] += 1; //Add one to the appropriate count
      last = 3*n + k; //Update last.
    }

    Prediction raw_predict() {
      if (DEBUG) DEBUG_FILE << "For " << last << ", Dual-Markov has " << counts[last][0] << "," << counts[last][1] << "," << counts[last][2] << '(' << mod << ')' << endl;
      Prediction r (counts[last][0], counts[last][1], counts[last][2]);
      return r;
    }
};

//Picks according to a search for the last 0-25 moves in the opponents' last 900 moves.
class StaticPatternHistory {
  private:
    int history[50];
    int mark;
    int _length;
  public:
    StaticPatternHistory() {
      mark = 0;
      _length = 0;
    }

    int operator[] (int n) {
      return (history[(mark + 50 - (n / 18)) % 50] % ((int) pow(3.0,n % 18 + 1))) / (int) pow(3.0,n % 18);
    }

    void push(int n) {
      if (n > 2 || n < 0) return;
      else {
        history[mark] *= 3;
        history[mark] += n;
        _length += 1;
        if (_length % 18 == 0) {
          if (_length == 900) _length -= 18;
          mark += 1;
          mark %= 50;
          history[mark] = 0;
        }
      }
    }
    
    int length() {
      return _length;
    }
};

//StaticPatternMatcher picks according to opponents' throws
class StaticPatternMatcher : public Predictor {
  private:
    StaticPatternHistory history;
  public:
    void feed(int n, int k) {
      history.push(n);
    }
    Prediction raw_predict () {
      int len = history.length();
      int last = history[0];
      int matched[] = {0,0,0};
      for (int i = 1; i < len; i += 1) {
        //Do the actual search
        for (int x = 0; history[i + x] == history[x] && x < 25; x += 1) matched[last] += x; //Weight goes up as the square of the length
        last = history[i]; //Update last
      }
      if (DEBUG) DEBUG_FILE << "Long pattern matcher has " << matched[0] << ',' << matched[1] << ',' << matched[2] << '(' << mod << ')' << endl;
      Prediction r (matched[0], matched[1], matched[2]);
      return r;
    }
};

//SelfStaticPatternMatcher picks according to our throws
class SelfStaticPatternMatcher : public Predictor {
  private:
    //TODO repeat memory; make access to a standard dual history.
    StaticPatternHistory our_history;
    StaticPatternHistory their_history;
  public:
    void feed (int n, int k) {
      their_history.push(n);
      our_history.push(k);
    }
    Prediction raw_predict () {
      int len = our_history.length();
      int last = their_history[0];
      int matched[] = {0,0,0};
      for (int i = 1; i < len; i += 1) {
        //Do the actual search
        for (int i = 0 ; i < len; i += 1) {
          for (int x = 0; our_history[i + x] == our_history[x] && x < 25; x += 1) matched[last] += x; //Weight goes up as the square of the length
          last = their_history[i]; //We mark the move that they made right after this pattern.
        }
        if (DEBUG) DEBUG_FILE << "Self pattern matcher has " << matched[0] << ',' << matched[1] << ',' << matched[2] << '(' << mod << ')' << endl;
        Prediction r (matched[0], matched[1], matched[2]);
        return r;
      }
    }
};

string stringify(int n, double* a) {
  stringstream s;
  s << '[';
  for (int i = 0; i < n - 1; i += 1) {
    s << a[i] << ',';
  }
  s << a[n - 1] << ']';
  return s.str();
}

class MovingAverage {
  private:
    double p[3]; //{win, tie, lose}
    double ratio;
  public:
    MovingAverage() {
      p[0] = 1; //We start our predictors with some confidence
      p[1] = 0;
      p[2] = 0;
      ratio = 0.9;
    }

    MovingAverage(double win, double tie, double lose) {
      p[0] = win;
      p[1] = tie;
      p[2] = lose;
      ratio = 0.9;
    }
    void feed(double n[3], int m) {
      for (int i = 0; i < 3; i += 1) {
        p[i] *= ratio;
        p[i] += (1 - ratio) * n[(m - i + 3) % 3];
        if (DEBUG) DEBUG_FILE << p[i] << ' ';
      }
      if (DEBUG) DEBUG_FILE << endl;
    }
    double expectation() {
      return p[0] - p[2];
    }
    string toString() {
      stringstream s;
      s << '[';
      for (int i = 0; i < 3; i += 1) s << p[i] << (i < 2? "," : "");
      s << ']';
      return s.str();
    }
    int shift() {
      if (p[0] - p[2] > p[1] - p[0] && p[0] - p[2] > p[2] - p[1]) {
        ratio = ratio * 0.9 + 0.09; //Our uncertainty in our predictors approaches 0.9 as they appear more reliable
        return 0;
      }
      else if (p[1] - p[0] > p[2] - p[1]) {
        if (DEBUG) DEBUG_FILE << "Going to flip (1). Array is " << stringify(3, p) << endl;
        ratio *= 0.8; //Morple gets very uncertain when a predictor has to swap
        double t = p[0];
        p[0] = p[1];
        p[1] = p[2];
        p[2] = t;
        if (DEBUG) DEBUG_FILE << "Flipped. New array is " << stringify(3, p) << endl;
        return 2;
      }
      else {
        if (DEBUG) DEBUG_FILE << "Going to flip (2). Array is " << stringify(3, p) << endl;
        ratio *= 0.8;
        double t = p[2];
        p[2] = p[1];
        p[1] = p[0];
        p[0] = t;
        if (DEBUG) DEBUG_FILE << "Flipped. New array is " << stringify(3, p) << endl;
        return 1;
      }
    }
};


int main() {
  int NUM_PREDICTORS = 6;
  int computer_score = 0;

//Predictors and their respective judges:
  Predictor* predictors[] = {new UnigramCounter(), new MarkovCounter(), new SelfMarkovCounter(), new DualMarkovCounter(), new StaticPatternMatcher(), new SelfStaticPatternMatcher};
  MovingAverage averages[NUM_PREDICTORS];

  //Aggregate prediction-related stuff:
  MovingAverage aggregate_average (1.0/3.0,1.0/3.0,1.0/3.0);
  int aggregate_shift = 0;

  //Last moves:
  int p;
  int g;

  int games = 0;
  while (true) {
    DEBUG_FILE << "Beginning game " << games << endl;
    Prediction predictions[NUM_PREDICTORS];
    Prediction aggregate (0,0,0);
    if (DEBUG) DEBUG_FILE << "----------" << endl;
    DEBUG_FILE << "Predicting: " << endl;
    for (int i = 0; i < NUM_PREDICTORS; i += 1) {
      DEBUG_FILE << i << ' ';
      //Get predictions:
      predictions[i] = predictors[i]->predict();
      aggregate += predictions[i].scaleTo(averages[i].expectation());
    }
    DEBUG_FILE << endl;
    aggregate.shift(aggregate_shift);
    DEBUG_FILE << "Got predictions for game " << games << endl;
    if (DEBUG) DEBUG_FILE << stringify(3, aggregate.as_array()) << endl;
    if (DEBUG) DEBUG_FILE << aggregate_average.toString() << endl;
    if (aggregate_average.expectation() > 0.15) {
      if (DEBUG) DEBUG_FILE << "I am confident. (" << aggregate_average.expectation() << ')' << endl;
      g = aggregate.generate();
    }
    else {
      if (DEBUG) DEBUG_FILE << "I am inconfident, and played randomly. (" << aggregate_average.expectation() << ')' << endl;
      g = rand() % 3;
    }
    if (DEBUG) DEBUG_FILE << "----------" << endl;
    DEBUG_FILE << "Ready to play game " << games << ". Going to play " << (g == 0 ? 'R' : (g == 1 ? 'P' : 'S')) << endl;
    cout << (g == 0 ? 'R' : (g == 1 ? 'P' : 'S'));
    char t;
    cin >> t;
    p = (t == 'R' ? 0 : (t == 'P' ? 1 : 2));
    for (int i = 0; i < NUM_PREDICTORS; i += 1) {
      //Feed everyone
      averages[i].feed(predictions[i].as_array(), p);
      predictors[i]->feed(p, g);
      //Shift if necessary
      predictors[i]->shift(averages[i].shift());
    }
    //Feed the aggregate average
    aggregate_average.feed(aggregate.as_array(), p);
    //Shift if necessary
    aggregate_shift += aggregate_average.shift();
    aggregate_shift %= 3;
    games += 1;
    DEBUG_FILE << "Ready to begin game " << games << endl;
  }
  return 0;
}
