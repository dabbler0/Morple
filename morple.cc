#include <iostream>
#include <cmath>
#include <sstream>
using namespace std;

/*
Morple

A basic rock-paper-scissors AI.

Morple has six predictors:

1. A unigram counter, which just counts word frequencies,
2,3. Two Markov modelers, one which constructs a Markov model on the opponent's history and one which constructs a Markov model on our history
4. A Markov modeler which constructs a Markov model on the combined history of us and the opponent,
5,6. Two large pattern searches, which look for repetitions of up to length 25, and predicts based on how many times the pattern was repeated and how long the pattern is. They search in our history and the opponent's history, respectively.

Morple makes these predictions and weights them according to how accurate they have been in the past. If any predictor is less than 50% accurate, Morple begins to bet against it instead.

Copyright (c) 2013 Anthony Bau.
*/

bool DEBUG = true;

class Prediction {
  private:
    double p[3];
    double t;
  public:
    Prediction(double rock, double paper, double scissors) {
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
      if (DEBUG) cout << "Unigrams has " << counts[0] << ',' << counts[1] << ',' << counts[2] << '(' << mod << ')' << endl;
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
      if (DEBUG) cout << "For " << last << ", Markov has " << counts[last][0] << "," << counts[last][1] << "," << counts[last][2] << '(' << mod << ')' << endl;
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
      if (DEBUG) cout << "For " << last << ", Self-Markov has " << counts[last][0] << ',' << counts[last][1] << ',' << counts[last][2] << '(' << mod << ')';
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
      if (DEBUG) cout << "For " << last << ", Dual-Markov has " << counts[last][0] << "," << counts[last][1] << "," << counts[last][2] << '(' << mod << ')' << endl;
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
      return (history[(mark + 50 - (n / 18)) % 50] % ((int) pow(3,n % 18 + 1))) / pow(3,n % 18);
    }

    void push(int n) {
      if (n > 2 || n < 0) return;
      else {
        history[mark] *= 3;
        history[mark] += n;
        _length += 1;
        if (_length % 18 == 0) {
          mark += 1;
          if (mark > 50) _length -= 18;
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
      if (DEBUG) cout << "Long pattern matcher has " << matched[0] << ',' << matched[1] << ',' << matched[2] << '(' << mod << ')' << endl;
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
        if (DEBUG) cout << "Self pattern matcher has " << matched[0] << ',' << matched[1] << ',' << matched[2] << '(' << mod << ')' << endl;
        Prediction r (matched[0], matched[1], matched[2]);
        return r;
      }
    }
};

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
    void feed(double n[3], int m) {
      cout << "Ratio is " << ratio << endl;
      for (int i = 0; i < 3; i += 1) {
        p[i] *= ratio;
        p[i] += (1 - ratio) * n[(m - i + 3) % 3];
      }
    }
    double expectation() {
      return p[0] - p[2];
    }
    string toString() {
      stringstream s;
      s << '(';
      for (int i = 0; i < 3; i += 1) s << p[i] << (i<2?",":"");
      s << ')';
      return s.str();
    }
    int shift() {
      if (p[0] - p[2] > p[1] - p[0] && p[0] - p[2] > p[2] - p[1]) {
        ratio = ratio * 0.9 + 0.09; //Our uncertainty in our predictors approaches 0.9 as they appear more reliable
        return 0;
      }
      else if (p[1] - p[0] > p[2] - p[1]) {
        ratio *= 0.8; //Morple gets very uncertain when a predictor has to swap
        double t = p[0];
        p[0] = p[1];
        p[1] = p[2];
        p[2] = t;
        return 1;
      }
      else {
        ratio *= 0.8;
        double t = p[1];
        p[0] = p[2];
        p[1] = p[0];
        p[2] = t;
        return 2;
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
  MovingAverage aggregate_average;
  int aggregate_shift = 0;

  char t;
  cout << 'P';
  while (true) {
    cin >> t;
    int p = (t == 'R' ? 0 : (t == 'P' ? 1 : t == 'S' ? 2 : -1));
    Prediction aggregate (0,0,0); //Init our aggregate Prediction
    if (DEBUG) cout << "-------------" << endl;
    for (int i = 0; i < NUM_PREDICTORS; i += 1) {
      //Cycle through our predictors, ask each one for a prediction, and then judge it.
      Prediction m = predictors[i]->predict();
      aggregate = aggregate + m.scaleTo(averages[i].expectation());
      averages[i].feed(m.as_array(), p); //Feed their scores to their expectation counters.
      if (DEBUG) cout << i << '|' << ' ' << averages[i].toString() << ' ' << averages[i].expectation() << endl;
      predictors[i]->shift(averages[i].shift()); //If they are doing badly, bet against them.
    }

    if (DEBUG) cout << "Aggregate is betting on " << aggregate[0] << ',' << aggregate[1] << ',' << aggregate[2] << endl;
    
    aggregate.shift(aggregate_shift);

    //Make our move
    int g = aggregate.generate();
    
    //Update scores:
    if ((g - p + 3) % 3 == 1) computer_score += 1;
    else if ((g - p + 3) % 3 == 2) computer_score -= 1;
    if (DEBUG) cout << "Score is now " << computer_score << endl;
    
    //Flip the aggregate counter if necessary.
    aggregate_average.feed(aggregate.as_array(), p);
    aggregate_shift += aggregate_average.shift();
    aggregate_shift %= 3;

    for (int i = 0; i < NUM_PREDICTORS; i += 1) predictors[i]->feed(p, g);

    if (DEBUG) cout << "------------" << endl << (g == 0 ? 'R' : (g == 1 ? 'P' : 'S')) << endl << endl;
    else cout << (g == 0 ? 'R' : (g == 1 ? 'P' : 'S')) << " (" << computer_score << ")" << endl << endl;
    //else cout << (g == 0 ? 'R' : (g == 1 ? 'P' : 'S'));
  }
  return 0;
}
