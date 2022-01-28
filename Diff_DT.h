/*
 ------------------------------------------------------------------------
|  This C4.5 code for CS570 is my own work written without consuting a   |
|  tutor  or code written by other students - Yuguang Meng, Oct 15, 2015 |
 ------------------------------------------------------------------------
*/

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>
#include <math.h>
#include <algorithm>

#define MAX_ATTR 32
#define MAX_H 3
#define K_FOLD 10
#define MAX(a,b) (a>b?a:b)

using namespace std;

int class_attr=0;
int q_f;   //quality function
float Epsilon;

enum Children_type
{
  NODE,
  VALUE
};

struct Node_DT;

typedef struct Child
{
  char attr_value[64];
  int type;
  union
  {
    Node_DT *node;
    char value[64];
  } node;
  int Leaf_count;
  double b;
  Child *next;
} Child;

typedef struct Node_DT
{
  int attr;
  Child *child;
} Node_DT;

typedef struct Node_data
{
  char value[MAX_ATTR][64];
  Node_data *next;
} Node_data;

typedef struct Node_class
{
  char *value;
  int count;
  Node_class *next;
} Node_class;

typedef struct Node_attr_value_class
{
  char *value;
  int count;
  Node_class *class_list;
  Node_attr_value_class *next;
} Node_attr_value_class;

typedef struct Attr_list
{
  int attr;
  Attr_list *next;
} Attr_list;

typedef struct Filter
{
  int attr;
  char value[64];
  Filter *next;
} Filter;

void Build_DT(Node_data *, Attr_list **, Filter *, Node_DT **, char **, int *, double);
void Clear_Filter(Filter **);
int Is_Data_Empty(Node_data *, Filter *);
void Build_Child_Filter(Filter *, Filter **, int, char *);
void Update_attr_list(Attr_list **, int);
void Attribute_Selection_Info_Gain(Node_data *, Filter *, Attr_list *, int *, Node_attr_value_class **, double b);
void Attribute_Selection_Gini_Index(Node_data *, Filter *, Attr_list *, int *, Node_attr_value_class **, double b);
void Attribute_Selection_Max(Node_data *, Filter *, Attr_list *, int *, Node_attr_value_class **, double b);
int Read_Data_Size(char *);
int Read_Data(Node_data **, Node_data **, int *, int, char *);
void Delete_dat_list(Node_data **dat);
void Delete_Node(Node_DT **Node);
int InsertData(Node_data **, char *);
void RemoveSpace(char *, char *);
int IsLetter(char);
int IsDigit(char);
void InsertAttrValueList(Node_attr_value_class **, char *, char *);
void InsertClassList(Node_class **, char *);
int Is_Same_Value(char **, Node_data *, Filter *);
int Is_Selected_Tuple(Node_data *, Filter *);
void Get_Majority_Class(Node_data *, Filter *, char **);
void Print_DT(FILE *, int *, Node_DT *, int);
void Read_Filter(Filter *);
void Calculate_Accuracy(FILE *, Node_DT *, Node_data *, int *, int *);
int Look_For_Class(Node_DT *, Node_data *, char **);
void Create_sub_attr_list(Attr_list **s_attr_list, Attr_list **d_attr_list);
void Delete_sub_attr_list(Attr_list **_attr_list);
int diff_EM(Attr_list *attr_list, double *q, double s_q, double e_b);
void Calculate_Noisy_Class_Counts(Node_DT *Node);
double Laplace(double s, double e_budget);
int *kfold(int, int);
void shuffleArray(int *, int);