#include "uuecode.h"

// type == 0 - fixed amount
// type == 1 - undefined amount
void addPart(char *text, int section, int amount, char* name, int type)
{
    char *begin = NULL, *end = NULL; 
    char *endstr = NULL;
    int partlen = 0;
    int rr = 0;
    UUEFile* node = NULL;
    w_log(LL_FUNC,"%s::addPart()", __FILE__);
    begin = strchr(text, '\r');
    if(!begin) return;
    
    while(begin[0] == '\r')
        begin++;

    end = begin;
    while( end && end[0] < '\x0061' && rr < 3)
    {
        rr = (end[0] == '\r') ?  rr+1 : 0;
        end++;
    }

    if(end)
    {
        if( rr > 1 ) end--; 
        partlen = end-begin;
        if(partlen < 12)
            return;

        endstr = strstr(end, "\rend\r");
    }
    else
        return;

    node = FindUUEFile(name);
    if(!node)
    {
        if( type && !endstr )
        {
            amount++;
        }
        node = MakeUUEFile(amount,name);
        node->prev = UFilesHead->prev;
        UFilesHead->prev->next = node; // add to end
        UFilesHead->prev       = node; // save end pos
    }
    else
    {
        if(type && section == node->m_nSections && !endstr)
        {
            node->m_nSections = section+1;
            node->UUEparts = (char**)srealloc( node->UUEparts , node->m_nSections*sizeof(char*) );
            if(nDelMsg || nCutMsg)
            {
                node->toBeDeleted = (dword*)srealloc(node->toBeDeleted,node->m_nSections*sizeof(dword));
            }
        }
    }
    AddPart(node, begin, section, partlen);
}

int scan4UUE(const char* text, dword textLen)
{
    int nRet = 0;
    char name[MAX];
    int perms;
    int section;
    int amount;
    int atype;
    float ff;
    int multi = 0;
    char *szSection = NULL;
    char *szBegin   = NULL;
    
    szSection = strstr(text, "section ");
    while(szSection)
    {
        if(sscanf(szSection,"section %d of %d of file %s",&section, &amount, name) == 3)
        {
            w_log(LL_FUNC,"%s::scan4UUE(), section %d of %d detected", __FILE__,section,amount);
            multi = 1;
            atype = 0;
        }
        else
        {
            if(sscanf(szSection,"section %d of uuencode %f of file %s",&section,&ff,name) == 3)
            {
                w_log(LL_FUNC,"%s::scan4UUE(), section %d detected", __FILE__, section);
                amount = section;
                multi = 1;
                atype = 1;
            }
            else
            {
                amount = 0;
            }
        }
        if(amount == 0) 
        {
            szSection = strstr(szSection+1, "section ");
            continue;
        }
        if(section == 1)
        {
            szBegin = strstr(szSection, "begin ");
            if(szBegin)
            {
                w_log(LL_FUNC,"%s::scan4UUE(), first section detected", __FILE__);
                addPart(szBegin, section, amount, name, atype);
            }
        }
        else
        {
            addPart(szSection, section, amount, name, atype);
        }
        szSection = strstr(szSection+1, "section ");
    }
    if(!multi)
    {
        szBegin = strstr(text, "begin ");
        while(szBegin)
        {
            if(sscanf(szBegin, "begin %o %s", &perms, name) == 2) {
                w_log(LL_FUNC,"%s::scan4UUE(), single message uue detcted", __FILE__);
                addPart(szBegin, 1, 1, name, 0);
            }
            szBegin = strstr(szBegin+1, "begin ");
        }
    }
    return nRet;
}

int processMsg(HAREA hArea, dword msgNumb)
{
   HMSG msg;
   char *text;
   dword  textLen;
   int unsent, rc = 0;


   msg = MsgOpenMsg(hArea, MOPEN_RW, msgNumb);
   if (msg == NULL) return rc;

   if (MsgReadMsg(msg, &xmsg, 0, 0, NULL, 0, NULL)<0) {
      MsgCloseMsg(msg);
      return rc;
   }

   unsent = (xmsg.attr & MSGLOCAL) && !(xmsg.attr & MSGSENT);
   currMsgUid = MsgMsgnToUid(hArea, msgNumb);

   textLen = MsgGetTextLen(msg);
   text = (char *) smalloc(textLen+1);
   text[textLen] = '\0';
   
   MsgReadMsg(msg, NULL, 0, textLen, (byte*)text, 0, NULL);
   MsgCloseMsg(msg);

   scan4UUE(text,textLen);
   nfree(text);
   rc = 1;
   
   return rc;
}

int cutUUEformMsg(HAREA hArea, dword msgNumb)
{
   HMSG msg;
   UCHAR  *text;
   dword  textLen;
   int rc = 0;


   msg = MsgOpenMsg(hArea, MOPEN_RW, msgNumb);
   if (msg == NULL) return rc;

   if (MsgReadMsg(msg, &xmsg, 0, 0, NULL, 0, NULL)<0) {
      MsgCloseMsg(msg);
      return rc;
   }

   textLen = MsgGetTextLen(msg);
   text = (UCHAR *) smalloc(textLen+1);
   text[textLen] = '\0';
   
   MsgReadMsg(msg, NULL, 0, textLen, (byte*)text, 0, NULL);
   text[0] = 's'; text[1] = 'e';text[2] = 'c';
   MsgWriteMsg(msg, 0, &xmsg, (byte*)text, textLen, textLen, 0, NULL);
   MsgCloseMsg(msg);

   nfree(text);
   rc = 1;
 
   return rc;
}