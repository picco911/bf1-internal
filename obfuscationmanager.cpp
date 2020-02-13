#include "obfuscationmgr.h"

fb::ClientPlayer* EncryptedPlayerMgr__GetPlayer(QWORD EncryptedPlayerMgr, int id)
{
    _QWORD XorValue1 = *(_QWORD*)(EncryptedPlayerMgr + 0x20) ^ *(_QWORD*)(EncryptedPlayerMgr + 8);
    _QWORD XorValue2 = (XorValue1 ^ *(_QWORD*)(EncryptedPlayerMgr + 0x10));
    _QWORD Player = XorValue1 ^ *(_QWORD*)(XorValue2 + 8 * id);

    return (fb::ClientPlayer*)Player;
}

fb::ClientPlayer* GetPlayerById(int id)
{
    fb::ClientGameContext* pClientGameContext = fb::ClientGameContext::GetInstance();
    if (!ValidPointer(pClientGameContext)) return nullptr;
    fb::ClientPlayerManager* pPlayerManager = pClientGameContext->m_clientPlayerManager;
    if (!ValidPointer(pPlayerManager)) return nullptr;

    _QWORD pObfuscationMgr = *(_QWORD*)OFFSET_ObfuscationMgr;
    //if (!ValidPointer(pObfuscationMgr)) return nullptr;

    _QWORD PlayerListXorValue = *(_QWORD*)((_QWORD)pPlayerManager + 0xF8);
    _QWORD PlayerListKey = PlayerListXorValue ^ *(_QWORD*)(pObfuscationMgr + 0x70);

    fb::hashtable<_QWORD>* table = (fb::hashtable<_QWORD>*)(pObfuscationMgr + 8);
    fb::hashtable_iterator<_QWORD> iterator = { 0 };

    fb::hashtable_find(table, &iterator, PlayerListKey);
    if (iterator.mpNode == table->mpBucketArray[table->mnBucketCount])
        return nullptr;

    _QWORD EncryptedPlayerMgr = (_QWORD)iterator.mpNode->mValue.second;
    if (!ValidPointer(EncryptedPlayerMgr)) return nullptr;

    _DWORD MaxPlayerCount = *(_DWORD*)(EncryptedPlayerMgr + 0x18);
    if (MaxPlayerCount != 70 || MaxPlayerCount <= id) return nullptr;

    return EncryptedPlayerMgr__GetPlayer(EncryptedPlayerMgr, id);
}

fb::ClientPlayer* GetLocalPlayer(void)
{
    fb::ClientGameContext* pClientGameContext = fb::ClientGameContext::GetInstance();
    if (!ValidPointer(pClientGameContext)) return nullptr;
    fb::ClientPlayerManager* pPlayerManager = pClientGameContext->m_clientPlayerManager;
    if (!ValidPointer(pPlayerManager)) return nullptr;

    _QWORD pObfuscationMgr = *(_QWORD*)OFFSET_ObfuscationMgr;
    //if (!ValidPointer(pObfuscationMgr)) return nullptr;

    _QWORD LocalPlayerListXorValue = *(_QWORD*)((_QWORD)pPlayerManager + 0xF0);
    _QWORD LocalPlayerListKey = LocalPlayerListXorValue ^ *(_QWORD*)(pObfuscationMgr + 0x70);

    fb::hashtable<_QWORD>* table = (fb::hashtable<_QWORD>*)(pObfuscationMgr + 8);
    fb::hashtable_iterator<_QWORD> iterator = { 0 };

    fb::hashtable_find(table, &iterator, LocalPlayerListKey);
    if (iterator.mpNode == table->mpBucketArray[table->mnBucketCount])
        return nullptr;

    _QWORD EncryptedPlayerMgr = (_QWORD)iterator.mpNode->mValue.second;
    if (!ValidPointer(EncryptedPlayerMgr)) return nullptr;

    _DWORD MaxPlayerCount = *(_DWORD*)(EncryptedPlayerMgr + 0x18);
    if (MaxPlayerCount != 1) return nullptr;

    return EncryptedPlayerMgr__GetPlayer(EncryptedPlayerMgr, 0);
}

fb::hashtable_iterator<_QWORD>* __fastcall fb::hashtable_find(fb::hashtable<_QWORD>* table, fb::hashtable_iterator<_QWORD>* iterator, _QWORD key)
{
    unsigned int mnBucketCount = table->mnBucketCount;

    //bfv
    //unsigned int startCount = (_QWORD)(key) % (_QWORD)(mnBucketCount);

    //bf1
    unsigned int startCount = (unsigned int)(key) % mnBucketCount;

    fb::hash_node<_QWORD>* node = table->mpBucketArray[startCount];

    if (ValidPointer(node) && node->mValue.first)
    {
        while (key != node->mValue.first)
        {
            node = node->mpNext;
            if (!node || !ValidPointer(node)
                )
                goto LABEL_4;
        }
        iterator->mpNode = node;
        iterator->mpBucket = &table->mpBucketArray[startCount];
    }
    else
    {
    LABEL_4:
        iterator->mpNode = table->mpBucketArray[mnBucketCount];
        iterator->mpBucket = &table->mpBucketArray[mnBucketCount];
    }
    return iterator;
}

void* DecryptPointer(_QWORD EncryptedPtr, _QWORD PointerKey)
{
    _QWORD pObfuscationMgr = *(_QWORD*)OFFSET_ObfuscationMgr;

    if (!ValidPointer(pObfuscationMgr))
        return nullptr;

    if (!(EncryptedPtr & 0x8000000000000000))
        return nullptr; //invalid ptr

    _QWORD hashtableKey = PointerKey ^ *(_QWORD*)(pObfuscationMgr + 0x70);

    fb::hashtable<_QWORD>* table = (fb::hashtable<_QWORD>*)(pObfuscationMgr + 0x38);
    fb::hashtable_iterator<_QWORD> iterator = {};

    fb::hashtable_find(table, &iterator, hashtableKey);
    if (iterator.mpNode == table->mpBucketArray[table->mnBucketCount])
        return nullptr;

    _QWORD EncryptionKey = hashtableKey ^ (_QWORD)(iterator.mpNode->mValue.second);
    EncryptionKey ^= (7 * EncryptionKey);

    _QWORD DecryptedPtr = NULL;
    BYTE* pDecryptedPtrBytes = (BYTE*)&DecryptedPtr;
    BYTE* pEncryptedPtrBytes = (BYTE*)&EncryptedPtr;
    BYTE* pKeyBytes = (BYTE*)&EncryptionKey;

    for (char i = 0; i < 7; i++)
    {
        pDecryptedPtrBytes[i] = (pKeyBytes[i] * 0x7A) ^ (pEncryptedPtrBytes[i] + pKeyBytes[i]);
        EncryptionKey += 8;
    }
    pDecryptedPtrBytes[7] = pEncryptedPtrBytes[7];

    DecryptedPtr &= ~(0x8000000000000000); //to exclude the check bit

    return (void*)DecryptedPtr;
}