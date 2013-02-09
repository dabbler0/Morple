#include <iostream>
#include <cmath>
using namespace std;

/*
MABEL

A naive rock-paper-scissors AI

Copyright (c) Anthony Bau.
*/

class Prediction {
  private:
    double p[3];
    double t;
  public:
    Prediction(double rock, double paper, double scissors) {
      //Beating probabilities, given opponent-throw probabilities
      p[1] = rock;
      p[2] = paper;
      p[0] = scissors;
      t = rock + paper + scissors;
    }
    void shift(int n) {
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

    int generate() {
      double point = ((double) rand() / (RAND_MAX)) * t;
      int r;
      if (point < p[0]) {
        r = 0;
      }
      else if (point < p[0] + p[1]) {
        r = 1;
      }
      else {
        r = 2;
      }
      return r;
    }
};

class Predictor {
  private:
    int mod;
  public:
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
    int counts[3];
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
      //cout << "Unigrams has " << counts[0] << ',' << counts[1] << ',' << counts[2] << endl;
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
      for (int i = 0; i < 3; i += 1) {
        for (int x = 0; x < 3; x += 1) {
          counts[i][x] = 0;
        }
      }
      last = -1;
    }
    void feed(int n, int k) {
      for (int i = 0; i < 3; i += 1) {
        for (int x = 0; x < 3; x += 1) {
          counts[i][x] *= 0.95;
        }
      }
      if (last >= 0) {
        counts[last][n] += 1;
      }
      last = n;
    }
    Prediction raw_predict() {
      //cout << "For " << last << ", Markov has " << counts[last][0] << "," << counts[last][1] << "," << counts[last][2] << endl;
      Prediction r (counts[last][0], counts[last][1], counts[last][2]);
      return r;
    }
};

//Picks according to a Markov model on tuples of throws (mine,theirs)
class DualMarkovCounter : public Predictor {
  private:
    int counts[9][3];
    int last;
  public:
    DualMarkovCounter() {
      for (int i = 0; i < 9; i += 1) {
        for (int x = 0; x < 3; x += 1) {
          counts[i][x] = 0;
        }
      }
      last = -1;
    }
    
    void feed(int n, int k) {
      for (int i = 0; i < 9; i += 1) {
        for (int x = 0; x < 3; x += 1) {
          counts[i][x] *= 0.96;
        }
      }
      if (last >= 0) {
        counts[last][n] += 1;
      }
      last = 3*n + k;
    }

    Prediction raw_predict() {
      //cout << "For " << last << ", Dual-Markov has " << counts[last][0] << "," << counts[last][1] << "," << counts[last][2] << endl;
      Prediction r (counts[last][0], counts[last][1], counts[last][2]);
      return r;
    }
};

//Picks according to a search for repeated substrings in the last 1000 moves, predictions weighted on substring length.
class StaticPatternHistory {
  private:
    int history[50]; //We can store up to 18 moves in a single integer
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
class StaticPatternMatcher : public Predictor {
  private:
    StaticPatternHistory history;
    int mark;
  public:
    void feed(int n, int k) {
      history.push(n);
    }
    Prediction raw_predict () {
      int len = history.length();
      int max_match = 0;
      int last = history[0];
      int matched[] = {0,0,0};
      for (int i = 1; i < len; i += 1) {
        for (int x = 0; history[i + x] == history[x]; x += 1) {
          matched[last] += 1;  
        }
        last = history[i];
      }
      //cout << "Long pattern matcher has last throws at " << history[0] << ' ' << history[1] << ' ' << history[2] << " ...so... " << matched[0] << ',' << matched[1] << ',' << matched[2] << endl;
      Prediction r (matched[0], matched[1], matched[2]);
      return r;
    }
};

class MovingAverage {
  private:
    double avg;
  public:
    MovingAverage() {
      avg = 0.05;
    }
    void feed(double p) {
      avg *= 0.8;
      avg += 0.2*p;
    }
    double average() {
      return avg;
    }
    void swap() {
      avg = - avg;
    }
};

int main() {
  int NUM_PREDICTORS = 4;
  
  Predictor* predictors[] = {new UnigramCounter(), new MarkovCounter(), new DualMarkovCounter(), new StaticPatternMatcher()};
  MovingAverage averages[NUM_PREDICTORS];
  char t;
  while (true) {
    cin >> t;
    int p = (t == 'R' ? 0 : (t == 'P' ? 1 : t == 'S' ? 2 : -1));
    
    Prediction aggregate (0,0,0);
    
    //cout << "-------------" << endl;

    for (int i = 0; i < NUM_PREDICTORS; i += 1) {
      Prediction m = predictors[i]->predict();
      aggregate = aggregate + m.scaleTo(averages[i].average());
      
      averages[i].feed(m[(p + 2) % 3] - m[(p + 1) % 3]);
      
      //cout << i << '|' << averages[i].average() << endl;

      if (averages[i].average() <= 0) {
        predictors[i]->shift(1);
        averages[i].swap();
      }
    }

    int g = aggregate.generate();
    
    for (int i = 0; i < NUM_PREDICTORS; i += 1) predictors[i]->feed(p, g);

    cout /*<< "------------" << endl*/ << (g == 0 ? 'R' : (g == 1 ? 'P' : 'S')) << endl << endl;

  }
  return 0;
}
