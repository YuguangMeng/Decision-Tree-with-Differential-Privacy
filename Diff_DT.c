/*
 ------------------------------------------------------------------------
|  This C4.5 code for CS570 is my own work written without consuting a   |
|  tutor  or code written by other students - Yuguang Meng, Oct 15, 2015 |
 ------------------------------------------------------------------------
*/

#include "Diff_DT.h"

int main(int argc, char ** argv)
{
  Node_DT *Node[K_FOLD];
  Node_data *train_dat=NULL, *test_dat=NULL;
  FILE *fp=NULL;
  int i, n_attr;
  Attr_list *attr_list=NULL, *p_attr_list=NULL;
  char *value=NULL;
  Filter *f=NULL;
  int return_type;
  int label[128];

  int dat_size=0;
  int *dat_indices=NULL;
  int j, n_total=0, n_correct=0;

  double acc_rate=0;

  if(argc!=3)
  {
    printf("Error: Usage: %s training_file test_file output_file!\n", argv[0]);
    exit(0);
  }

  printf("\n");
  printf(" ------------------------------------------------------------------------\n");
  printf("| This code for CS573 project is my own work written without consuting   |\n");
  printf("| a tutor  or code written by other students - Yuguang Meng, Nov 15 2016 |\n");
  printf(" ------------------------------------------------------------------------\n");
  printf("\n");

  dat_size=Read_Data_Size(argv[1]);
  dat_indices=kfold(K_FOLD, dat_size);

  printf("The data file is:   %s.\n", argv[1]);

  do
  {
    printf("Please enter No. of the class attribute colume: ");
    scanf ("%d", &class_attr);
  } while(class_attr<1);

  do
  {
    printf("Please enter the budge for differential privacy (0~100): ");
    scanf ("%f", &Epsilon);
  } while(Epsilon<=0.0 || Epsilon>100.0);

  do
  {
    printf("Please select the quality function (1: infomation gain; 2: Gini index; 3: Max): ");
    scanf ("%d", &q_f);
  } while(q_f<1 || q_f>3);

  class_attr--;

  for(j=0; j<K_FOLD; j++)
  {
    n_attr=Read_Data(&train_dat, &test_dat, dat_indices, j, argv[1]);
    if(n_attr<=class_attr)
    {
      printf("Incorrect column of class attribute!\n");
      exit(0);
    }
    for(i=0; i<n_attr; i++)
    {
      p_attr_list=(Attr_list *)malloc(sizeof(Attr_list));
      p_attr_list->attr=i;
      p_attr_list->next=NULL;
      if(!attr_list)
        attr_list=p_attr_list;
      else
      {
        p_attr_list->next=attr_list;
        attr_list=p_attr_list;
      }
    }
    //Delete_Node(&Node);
    Build_DT(train_dat, &attr_list, f, &Node[j], &value, &return_type, Epsilon/2.0);
    Calculate_Accuracy(fp, Node[j], test_dat, &n_total, &n_correct);
    Calculate_Noisy_Class_Counts(Node[j]);

    acc_rate+=100.0*(double)n_correct/(double)n_total;
    //printf("%f\n", 100.0*(double)n_correct/(double)n_total);
    Delete_sub_attr_list(&attr_list);
    Delete_dat_list(&train_dat);
    Delete_dat_list(&test_dat);
  }


  if(!(fp=fopen(argv[2],"w")))
  {
    printf("Cannot create output file %s!\n", argv[2]);
    exit(0);
  }

  printf("\nThe decision tree:\n\n");
  fprintf(fp, "The decision tree:\n\n");
  if(return_type==VALUE)
  {
    printf("%s\n", value);
    fprintf(fp, "%s\n", value);
  }
  if(return_type==NODE)
  {
    for(i=0; i<128; i++)
      label[i]=0;
    Print_DT(fp, label, Node[0], 0);
  }

  printf("\nThe accuracy for the data is %.1f%% after 10-Fold cross validation.\n", acc_rate/10.0);
  fprintf(fp, "\nThe accuracy for the data is %.1f%% after 10-Fold cross validation.\n", acc_rate/10.0);
  fclose(fp);

  printf("\nResults saved to %s.\n\n", argv[2]);

  return 0;
}

void Print_DT(FILE *fp, int *label, Node_DT *Node, int n_tab)
{
  int i;
  Child *p_child=NULL;

  p_child=Node->child;

  while(p_child)
  {
    for(i=0; i<n_tab; i++)
    {
      if(label[i])
      {
        printf("|");
        fprintf(fp, "|");
      }
      printf("\t");
      fprintf(fp, "\t");
    }
    if(p_child->type==VALUE)
    {
      printf("(%d) = %s: %s (%d)\n", Node->attr+1, p_child->attr_value, p_child->node.value, p_child->Leaf_count);
      fprintf(fp, "(%d) = %s: %s (%d)\n", Node->attr+1, p_child->attr_value, p_child->node.value, p_child->Leaf_count);
    }
    if(p_child->type==NODE)
    {
      printf("(%d) = %s\n", Node->attr+1, p_child->attr_value);
      fprintf(fp, "(%d) = %s\n", Node->attr+1, p_child->attr_value);
      if(p_child->next)
        label[n_tab]=1;
      Print_DT(fp, label, p_child->node.node, n_tab+1);
    }
    p_child=p_child->next;
  }
}

void Calculate_Accuracy(FILE *fp, Node_DT *Node, Node_data *dat, int *n_total, int *n_correct)
{
  int r;
  char *value=NULL;
  while(dat)
  {
    (*n_total)++;
    r=Look_For_Class(Node, dat, &value);
    //fprintf(fp, "%s ", value);
    if(r)
      (*n_correct)++;
    dat=dat->next;
  }
}

int Look_For_Class(Node_DT *Node, Node_data *dat, char **value)
{
  Child *p_child=Node->child;
  while(p_child)
  {
    if(strcmp(dat->value[Node->attr], p_child->attr_value)==0)
    {
      if(p_child->type==VALUE)
      {
        (*value)=(char *)malloc(strlen(p_child->node.value));
        strcpy(*value, p_child->node.value);
        p_child->Leaf_count++;
        if(strcmp(dat->value[class_attr], p_child->node.value)==0)
          return 1;
        else
          return 0;
      }
      if(p_child->type==NODE)
        return Look_For_Class(p_child->node.node, dat, value);
    }
    else
      p_child=p_child->next;
  }
}

void Calculate_Noisy_Class_Counts(Node_DT *Node)
{
  Child *p_child=Node->child;
  while(p_child)
  {
    if(p_child->type==VALUE)
    {

      p_child->Leaf_count=p_child->Leaf_count+Laplace(1.0, p_child->b);
      p_child->Leaf_count=MAX(p_child->Leaf_count, 0);
    }
    if(p_child->type==NODE)
      Calculate_Noisy_Class_Counts(p_child->node.node);
    p_child=p_child->next;
  }
}

void Build_DT(Node_data *dat, Attr_list **attr_list, Filter *f, Node_DT **Node, char **value, int *return_type, double b)
{
  int splitting_attr;
  Node_DT *Node_child=NULL;
  char *value_child=NULL;
  int return_type_child;
  Node_attr_value_class *splitting_attr_value_list=NULL;
  Filter *f_child=NULL;
  Child *p_child=NULL;
  Node_class *p_class_list=NULL;
  Attr_list *sub_attr_list=NULL;

  if(Is_Same_Value(value, dat, f))
  {
    *return_type=VALUE;
    return;
  }

  if(!(*attr_list) || b<0)
  {
    Get_Majority_Class(dat, f, value);
    *return_type=VALUE;
    return;
  }

  if(q_f==1)
    Attribute_Selection_Info_Gain(dat, f, *attr_list, &splitting_attr, &splitting_attr_value_list, Epsilon/(2.0*MAX_H));  //Epsilon/(2.0*MAX_H)
  if(q_f==2)
    Attribute_Selection_Gini_Index(dat, f, *attr_list, &splitting_attr, &splitting_attr_value_list, Epsilon/(2.0*MAX_H));
  if(q_f==3)
    Attribute_Selection_Max(dat, f, *attr_list, &splitting_attr, &splitting_attr_value_list, Epsilon/(2.0*MAX_H));

  *Node=(Node_DT *)malloc(sizeof(Node_DT));
  *return_type=NODE;
  (*Node)->attr=splitting_attr;

  Update_attr_list(attr_list, splitting_attr);

  p_child=(Child *)malloc(sizeof(Child));
  p_child->next=NULL;
  (*Node)->child=p_child;

  while(splitting_attr_value_list)
  {
    strcpy(p_child->attr_value, splitting_attr_value_list->value);
    Build_Child_Filter(f, &f_child, splitting_attr, splitting_attr_value_list->value);
    if(Is_Data_Empty(dat, f_child))
    {
      Get_Majority_Class(dat, f, value);
      *return_type=VALUE;
    }
    else
    {
      Create_sub_attr_list(attr_list, &sub_attr_list);
      Build_DT(dat, &sub_attr_list, f_child, &Node_child, &value_child, &return_type_child, b-Epsilon/(2.0*MAX_H));
      Delete_sub_attr_list(&sub_attr_list);
    }

    p_child->type=return_type_child;
    if(return_type_child==VALUE)
    {
      if(b-Epsilon/(2.0*MAX_H)>=0)
        p_child->b=Epsilon/2.0+b-Epsilon/(2.0*MAX_H);
      if(b-Epsilon/(2.0*MAX_H)<0)
        p_child->b=Epsilon/2.0+b;
      strcpy(p_child->node.value, value_child);
      p_child->Leaf_count=0;
    }
    if(return_type_child==NODE)
      p_child->node.node=Node_child;

    Clear_Filter(&f_child);
    if(splitting_attr_value_list->next)
    {
      p_child->next=(Child *)malloc(sizeof(Child));
      p_child=p_child->next;
      p_child->next=NULL;
    }

    splitting_attr_value_list=splitting_attr_value_list->next;
  }
}

void Read_Filter(Filter *f)
{
  printf("Filter------------------------Start\n");
  while(f)
  {
    printf("attr=%d, value=%s\n", f->attr, f->value);
    f=f->next;
  }
  printf("Filter------------------------End\n");
}

void Clear_Filter(Filter **f)
{
  Filter *p_f=*f, *p_f_prev;
  while(p_f)
  {
    p_f_prev=p_f;
    p_f=p_f->next;
    free(p_f_prev);
  }
}

int Is_Data_Empty(Node_data *pdat, Filter *f)
{
  while(pdat)
  {
    if(Is_Selected_Tuple(pdat, f))
      return 0;
    pdat=pdat->next;
  }
  return 1;
}

void Build_Child_Filter(Filter *f, Filter **f_child, int splitting_attr, char *value)
{
  Filter *p_f=NULL;

  p_f=(Filter *)malloc(sizeof(Filter));
  *f_child=p_f;

  while(f)
  {
    p_f->attr=f->attr;
    strcpy(p_f->value, f->value);
    p_f->next=NULL;
    p_f->next=(Filter *)malloc(sizeof(Filter));
    p_f=p_f->next;
    f=f->next;
  }

  p_f->attr=splitting_attr;
  strcpy(p_f->value, value);
  p_f->next=NULL;
}

void Update_attr_list(Attr_list **attr_list, int splitting_attr)
{
  Attr_list *p_attr_list=*attr_list, *p_attr_list_prev=NULL;
  while(p_attr_list)
  {
    if(p_attr_list->attr==splitting_attr)
    {
      if(!p_attr_list_prev)
        *attr_list=p_attr_list->next;
      else
        p_attr_list_prev->next=p_attr_list->next;
      free(p_attr_list);
      break;
    }
    p_attr_list_prev=p_attr_list;
    p_attr_list=p_attr_list->next;
  }
}

void Create_sub_attr_list(Attr_list **s_attr_list, Attr_list **d_attr_list)
{
  Attr_list *p_s_attr_list=*s_attr_list;
  Attr_list *p_d_attr_list=*d_attr_list;
  while(p_s_attr_list)
  {
    p_d_attr_list=(Attr_list *)malloc(sizeof(Attr_list));
    p_d_attr_list->attr=p_s_attr_list->attr;
    p_d_attr_list->next=NULL;
    if(*d_attr_list==NULL)
      *d_attr_list=p_d_attr_list;
    else
    {
      p_d_attr_list->next=(*d_attr_list)->next;
      (*d_attr_list)->next=p_d_attr_list;
    }
    p_s_attr_list=p_s_attr_list->next;
  }
}

void Delete_sub_attr_list(Attr_list **attr_list)
{
  Attr_list *p_attr_list=*attr_list;
  Attr_list *temp=NULL;
  while(p_attr_list)
  {
    temp=p_attr_list->next;
    free(p_attr_list);
    p_attr_list=temp;
  }
  *attr_list=NULL;
}

void Delete_dat_list(Node_data **dat)
{
  Node_data *p_dat=*dat;
  Node_data *temp=NULL;
  while(p_dat)
  {
    temp=p_dat->next;
    free(p_dat);
    p_dat=temp;
  }
  *dat=NULL;
}

void Delete_Node(Node_DT **Node)
{
  Node_DT *p=*Node;
  Child *p_child=NULL, *temp=NULL;
  if(p==NULL)
    return;
  if(p->child!=NULL)
  {
    p_child=p->child;
    while(p_child)
    {
      if(p_child->type==NODE)
        Delete_Node(&(p_child->node.node));
      temp=p_child->next;
      free(p_child);
      p_child=temp;
    }
  }
  else
    free(p);
  *Node=NULL;
}

void Attribute_Selection_Info_Gain(Node_data *dat, Filter *f, Attr_list *attr_list,
                                   int *splitting_attr, Node_attr_value_class **splitting_attr_value_list, double b)
{
  Node_data *pdat;
  Attr_list *p_attr_list;
  Node_class *class_list=NULL, *p_class_list=NULL;
  Node_attr_value_class *attr_value_class_list[MAX_ATTR], *p_attr_value_class_list;
  int i;
  double p, N=0, Info_D_A_sub=0, Info_D_A=0, *qIG_A=NULL, s_qIG;

  //Calculate N
  pdat=dat;
  while(pdat)
  {
    InsertClassList(&class_list, pdat->value[class_attr]);
    pdat=pdat->next;
  }

  p_class_list=class_list;
  while(p_class_list)
  {
    N+=p_class_list->count;
    p_class_list=p_class_list->next;
  }

  qIG_A=(double *)malloc(sizeof(double)*MAX_ATTR);
  p_attr_list=attr_list;
  while(p_attr_list)
  {
    qIG_A[p_attr_list->attr]=0;
    p_attr_list=p_attr_list->next;
  }

  //Calculate Info_D_A
  p_attr_list=attr_list;
  while(p_attr_list)
  {
    if(p_attr_list->attr==class_attr)
    {
      p_attr_list=p_attr_list->next;
      continue;
    }

    attr_value_class_list[p_attr_list->attr]=NULL;
    pdat=dat;
    Info_D_A=0;

    while(pdat)
    {
      if(Is_Selected_Tuple(pdat, f))
        InsertAttrValueList(&attr_value_class_list[p_attr_list->attr],
                            pdat->value[p_attr_list->attr],
                            pdat->value[class_attr]);
      pdat=pdat->next;
    }

    p_attr_value_class_list=attr_value_class_list[p_attr_list->attr];
    while(p_attr_value_class_list)
    {
      Info_D_A_sub=0;
      p_class_list=p_attr_value_class_list->class_list;
      while(p_class_list)
      {
        p=(double)(p_class_list->count)/(double)(p_attr_value_class_list->count);
        Info_D_A_sub+=(double)(p_class_list->count)*log(p)/log(2.0);
        p_class_list=p_class_list->next;
      }
      Info_D_A+=Info_D_A_sub;
      p_attr_value_class_list=p_attr_value_class_list->next;
    }
    qIG_A[p_attr_list->attr]=-Info_D_A;
    p_attr_list=p_attr_list->next;
  }

  s_qIG=log(N+1)/log(2.0)+1.0/log(2.0);
  i=diff_EM(attr_list, qIG_A, s_qIG, b);
  *splitting_attr=i;
  *splitting_attr_value_list=attr_value_class_list[i];
  free(qIG_A);
}

void Attribute_Selection_Gini_Index(Node_data *dat, Filter *f, Attr_list *attr_list,
                                    int *splitting_attr, Node_attr_value_class **splitting_attr_value_list, double b)
{
  Node_data *pdat;
  Attr_list *p_attr_list;
  Node_class *p_class_list=NULL;
  Node_attr_value_class *attr_value_class_list[MAX_ATTR], *p_attr_value_class_list;
  int i;
  double p, Info_D_A_sub=0, Info_D_A=0, *qGini_A=NULL, s_qGini;

  qGini_A=(double *)malloc(sizeof(double)*MAX_ATTR);
  p_attr_list=attr_list;
  while(p_attr_list)
  {
    qGini_A[p_attr_list->attr]=0;
    p_attr_list=p_attr_list->next;
  }

  //Calculated Info_D_A
  p_attr_list=attr_list;
  while(p_attr_list)
  {
    if(p_attr_list->attr==class_attr)
    {
      p_attr_list=p_attr_list->next;
      continue;
    }

    attr_value_class_list[p_attr_list->attr]=NULL;
    pdat=dat;
    Info_D_A=0;

    while(pdat)
    {
      if(Is_Selected_Tuple(pdat, f))
        InsertAttrValueList(&attr_value_class_list[p_attr_list->attr],
                            pdat->value[p_attr_list->attr],
                            pdat->value[class_attr]);
      pdat=pdat->next;
    }

    p_attr_value_class_list=attr_value_class_list[p_attr_list->attr];
    while(p_attr_value_class_list)
    {
      Info_D_A_sub=0;
      p_class_list=p_attr_value_class_list->class_list;
      while(p_class_list)
      {
        p=(double)(p_class_list->count)/(double)(p_attr_value_class_list->count);
        Info_D_A_sub+=pow(p, 2.0);
        p_class_list=p_class_list->next;
      }
      Info_D_A_sub=(double)(p_attr_value_class_list->count)*(1.0-Info_D_A_sub);
      Info_D_A+=Info_D_A_sub;
      p_attr_value_class_list=p_attr_value_class_list->next;
    }
    qGini_A[p_attr_list->attr]=-Info_D_A;
    p_attr_list=p_attr_list->next;
  }

  s_qGini=2.0;
  i=diff_EM(attr_list, qGini_A, s_qGini, b);
  *splitting_attr=i;
  *splitting_attr_value_list=attr_value_class_list[i];
  free(qGini_A);
}

void Attribute_Selection_Max(Node_data *dat, Filter *f, Attr_list *attr_list,
                             int *splitting_attr, Node_attr_value_class **splitting_attr_value_list, double b)
{
  Node_data *pdat;
  Attr_list *p_attr_list;
  Node_class *p_class_list=NULL;
  Node_attr_value_class *attr_value_class_list[MAX_ATTR], *p_attr_value_class_list;
  int i;
  double Info_D_A_sub=0, Info_D_A=0, *qMax_A=NULL, s_qMax;

  qMax_A=(double *)malloc(sizeof(double)*MAX_ATTR);
  p_attr_list=attr_list;
  while(p_attr_list)
  {
    qMax_A[p_attr_list->attr]=0;
    p_attr_list=p_attr_list->next;
  }

  //Calculated Info_D_A
  p_attr_list=attr_list;
  while(p_attr_list)
  {
    if(p_attr_list->attr==class_attr)
    {
      p_attr_list=p_attr_list->next;
      continue;
    }

    attr_value_class_list[p_attr_list->attr]=NULL;
    pdat=dat;
    Info_D_A=0;

    while(pdat)
    {
      if(Is_Selected_Tuple(pdat, f))
        InsertAttrValueList(&attr_value_class_list[p_attr_list->attr],
                            pdat->value[p_attr_list->attr],
                            pdat->value[class_attr]);
      pdat=pdat->next;
    }

    p_attr_value_class_list=attr_value_class_list[p_attr_list->attr];
    while(p_attr_value_class_list)
    {
      Info_D_A_sub=0;
      p_class_list=p_attr_value_class_list->class_list;
      while(p_class_list)
      {
        if(Info_D_A_sub<(double)(p_class_list->count))
          Info_D_A_sub=(double)(p_class_list->count);
        p_class_list=p_class_list->next;
      }
      Info_D_A+=Info_D_A_sub;
      p_attr_value_class_list=p_attr_value_class_list->next;
    }
    qMax_A[p_attr_list->attr]=Info_D_A;
    p_attr_list=p_attr_list->next;
  }

  s_qMax=1.0;
  i=diff_EM(attr_list, qMax_A, s_qMax, b);
  *splitting_attr=i;
  *splitting_attr_value_list=attr_value_class_list[i];
  free(qMax_A);
}

int diff_EM(Attr_list *attr_list, double *q, double s_q, double e_b)
{
  Attr_list *p_attr_list=NULL, *p_t=NULL;
  double max=0.0, sum=0.0, r;
  double *p=NULL, p_c=0;
  double *buf=NULL;
  int i=0, max_attr, attr_list_len=0;

  p=(double *)malloc(sizeof(double)*MAX_ATTR);
  p_attr_list=attr_list;
  while(p_attr_list)
  {
    if(p_attr_list->attr==class_attr)
    {
      p_attr_list=p_attr_list->next;
      continue;
    }
    p[p_attr_list->attr]=e_b*q[p_attr_list->attr]/(2.0*s_q);
    if(p[p_attr_list->attr]>max)
    {
      max=p[p_attr_list->attr];
      max_attr=p_attr_list->attr;
    }
    p_attr_list=p_attr_list->next;
    attr_list_len++;
  }

  p_attr_list=attr_list;
  while(p_attr_list)
  {
    if(p_attr_list->attr==class_attr)
    {
      p_attr_list=p_attr_list->next;
      continue;
    }
    p[p_attr_list->attr]=exp(p[p_attr_list->attr]-max);
    sum+=p[p_attr_list->attr];
    p_attr_list=p_attr_list->next;
  }

  r=(double)rand()/RAND_MAX;
  sum*=r;

  buf=(double *)calloc(attr_list_len,sizeof(double));
  p_attr_list=attr_list;
  while(p_attr_list)
  {
    if(p_attr_list->attr==class_attr)
    {
      p_attr_list=p_attr_list->next;
      continue;
    }
    buf[i]=p[p_attr_list->attr];
    i++;
    p_attr_list=p_attr_list->next;
  }
  std::sort(&buf[0], &buf[attr_list_len]);

  for(i=0; i<attr_list_len; i++)
  {
    sum-=buf[i];
    if(sum<=0.0)
    {
      p_attr_list=attr_list;
      while(p_attr_list)
      {
        if(p_attr_list->attr==class_attr)
        {
          p_attr_list=p_attr_list->next;
          continue;
        }
        if(buf[i]==p[p_attr_list->attr])
        {
          free(buf);
          return p_attr_list->attr;
        }
        p_attr_list=p_attr_list->next;
      }
    }
  }
  return max_attr;
}

int Read_Data_Size(char *filename)
{
  FILE *fp;
  int dat_size=0;
  char buf[256];

  if((fp=fopen(filename,"r"))==NULL)
  {
    printf("File %s not exist!\n", filename);
    exit(0);
  }

  while(fgets (buf, 256, fp)!=NULL)
    dat_size++;
  fclose(fp);
  return dat_size;
}

int Read_Data(Node_data **dat_train, Node_data **dat_test, int *dat_indices, int k, char *filename)
{
  FILE *fp;
  char buf[256];
  int i=0, n_attr;

  if((fp=fopen(filename,"r"))==NULL)
  {
    printf("File %s not exist!\n", filename);
    exit(0);
  }

  while(fgets (buf, 256, fp)!=NULL)
  {
    if(dat_indices[i]!=k)
      n_attr=InsertData(dat_train, buf);
    if(dat_indices[i]==k)
      n_attr=InsertData(dat_test, buf);
    i++;
  }

  fclose(fp);
  return n_attr;
}

int InsertData(Node_data **dat, char *s)
{
  int i=0;
  Node_data *pInsert;
  Node_data *pHead=*dat;

  char *p, *str, buf[256];
  const char *d = " \t\n";

  pInsert = (Node_data *)malloc(sizeof(Node_data));
  pInsert->next=NULL;

  strcpy(buf,s);
  p = strtok(buf,d);
  while(p)
  {
    str=(char *)malloc(strlen(p)+1);
    RemoveSpace(str, p);
    if(strlen(str)!=0)
    {
      strcpy(pInsert->value[i], str);
      i++;
    }
    free(str);
    p=strtok(NULL,d);
  }

  if(!pHead)
    *dat=pInsert;
  else
  {
    while(pHead->next)
      pHead=pHead->next;
    pHead->next=pInsert;
  }
  return i;
}

void RemoveSpace(char *s, char *p)
{
  char *p1=p;
  while(*p1!='\0')
  {
    if(IsLetter(*p1) || IsDigit(*p1) || (*p1)=='-' || (*p1)=='_')
    {
      *s=*p1;
      s++;
    }
    p1++;
  }
  *s='\0';
}

int IsLetter(char c) //Letter
{
  if(((c<='z')&&(c>='a'))||((c<='Z')&&(c>='A')))
    return 1;
  else
    return 0;
}

int IsDigit(char c) //Digit
{
  if(c>='0'&&c<='9')
    return 1;
  else
    return 0;
}

void InsertAttrValueList(Node_attr_value_class **pNode, char *attr_value, char *class_value)
{
  Node_attr_value_class *pInsert=NULL;
  Node_attr_value_class *pHead=NULL, *pHead_prev=NULL;
  Node_class *pClass=NULL;

  pHead = *pNode;

  pInsert = (Node_attr_value_class *)malloc(sizeof(Node_attr_value_class));
  pInsert->value = (char *)malloc(strlen(attr_value)+1);
  strcpy(pInsert->value, attr_value);
  pInsert->count=1;
  pInsert->next=NULL;

  InsertClassList(&pClass, class_value);
  pInsert->class_list=pClass;

  if(!pHead)
  {
    pHead = pInsert;
    *pNode=pHead;
  }
  else
  {
    while(pHead)
      if(strcmp(pHead->value, pInsert->value)==0)
      {
        (pHead->count)++;
        InsertClassList(&pHead->class_list, class_value);
        free(pInsert);
        if(pClass)
          free(pClass);
        return;
      }
      else
      {
        pHead_prev=pHead;
        pHead=pHead->next;
      }
    pHead_prev->next = pInsert;
  }
}

void InsertClassList(Node_class **pNode, char *value)
{
  int i;
  Node_class *pInsert=NULL;
  Node_class *pHead, *pHead_prev=NULL;

  pHead = *pNode;

  pInsert = (Node_class *)malloc(sizeof(Node_class));
  pInsert->value = (char *)malloc(strlen(value)+1);
  strcpy(pInsert->value, value);
  pInsert->count=1;
  pInsert->next=NULL;

  if(!pHead)
  {
    pHead = pInsert;
    *pNode=pHead;
  }
  else
  {
    while(pHead)
      if(strcmp(pHead->value, pInsert->value)==0)
      {
        (pHead->count)++;
        free(pInsert);
        return;
      }
      else
      {
        pHead_prev=pHead;
        pHead=pHead->next;
      }
    pHead_prev->next = pInsert;
  }
}

int Is_Same_Value(char **value, Node_data *dat, Filter *f)
{
  Node_data *pdat=dat;
  while(pdat)
    if(Is_Selected_Tuple(pdat, f))
    {
      *value=(char *)malloc(sizeof(pdat->value[class_attr])+1);
      strcpy(*value, pdat->value[class_attr]);
      break;
    }
    else
      pdat=pdat->next;

  pdat=dat;
  while(pdat)
  {
    if(Is_Selected_Tuple(pdat, f))
      if(strcmp(pdat->value[class_attr], *value)!=0)
      {
        free(*value);
        return 0;
      }
    pdat=pdat->next;
  }
  return 1;
}

int Is_Selected_Tuple(Node_data *pdat, Filter *f)
{
  while(f)
  {
    if(strcmp(pdat->value[f->attr], f->value)!=0)
      return 0;
    f=f->next;
  }
  return 1;
}

void Get_Majority_Class(Node_data *pdat, Filter *f, char **majority_class)
{
  Node_class *class_list=NULL, *class_list_head=NULL, *class_list_prev=NULL;
  int n_majority_class;

  while(pdat)
  {
    if(Is_Selected_Tuple(pdat, f))
      InsertClassList(&class_list, pdat->value[class_attr]);
    pdat=pdat->next;
  }

  class_list_head=class_list;

  n_majority_class=class_list->count;
  *majority_class=(char *)malloc(16);
  strcpy(*majority_class, class_list->value);
  while(class_list)
  {
    if(class_list->count>n_majority_class)
    {
      //free(majority_class);
      //*majority_class=(char *)malloc(sizeof(class_list->value)+1);
      strcpy(*majority_class, class_list->value);
      n_majority_class=class_list->count;
    }
    class_list=class_list->next;
  }

  class_list=class_list_head;
  while(class_list)
  {
    class_list_prev=class_list;
    class_list=class_list->next;
    free(class_list_prev);
  }
}

double Laplace(double s, double e_budget)
{
  double u,b;
  u=(double)rand()/RAND_MAX-0.5;
  b=s/e_budget;
  return -b*(u/fabs(u))*log(1-2*fabs(u));
}

int* kfold(int k, int dat_size)
{
  int *indices = new int[ dat_size ];

  for (int i = 0; i < dat_size; i++ )
    indices[ i ] = i%k;

  shuffleArray(indices, dat_size);

  return indices;
}

void shuffleArray(int *array, int size)
{
  int n = size;
  while (n)
  {
    // 0 <= k < n.
    int k = rand()%n;

    // n is now the last pertinent index;
    n--;

    // swap array[n] with array[k]
    int temp = array[n];
    array[n] = array[k];
    array[k] = temp;
  }
}