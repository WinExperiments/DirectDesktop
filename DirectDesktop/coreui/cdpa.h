#pragma once

template<typename T, typename ContainerPolicy>
class CDPA_Base {
public:
    using EnumCallbackType = int (*)(T*, void*);
    using CompareType = int (*)(T* p1, T* p2, LPARAM lParam);
    using MergeType = T * (*)(UINT uMsg, T* pDest, T* pSrc, LPARAM lParam);

    CDPA_Base(HDPA hdpa = nullptr) : m_hdpa(hdpa) {
    }

    ~CDPA_Base() {
        if (m_hdpa)
            Destroy();
    }

    BOOL IsDPASet() const {
        return m_hdpa != nullptr;
    }

    void Attach(HDPA hdpa) {
        m_hdpa = hdpa;
    }
    HDPA Detach() {
        HDPA hdpa = m_hdpa;
        m_hdpa = nullptr;
        return hdpa;
    }

    operator HDPA() const {
        return m_hdpa;
    }

    BOOL Create(int cItemGrow) {
        m_hdpa = DPA_Create(cItemGrow);
        return m_hdpa != nullptr;
    }

    BOOL CreateEx(int cpGrow, HANDLE hheap) {
        m_hdpa = DPA_CreateEx(cpGrow, hheap);
        return m_hdpa != nullptr;
    }
    BOOL Destroy() {
        BOOL result = TRUE;
        if (m_hdpa) {
            DestroyCallback(_StandardDestroyCB, nullptr);
            result = DPA_Destroy(m_hdpa);
            m_hdpa = nullptr;
        }
        return result;
    }
    HDPA Clone(HDPA hdpaNew) const {
        return DPA_Clone(m_hdpa, hdpaNew);
    }

    T* GetPtr(INT_PTR i) const {
        return (T*)DPA_GetPtr(m_hdpa, i);
    }
    int GetPtrIndex(T* p) {
        return DPA_GetPtrIndex(m_hdpa, p);
    }
    BOOL Grow(int cp) {
        return DPA_Grow(m_hdpa, cp);
    }
    BOOL SetPtr(int i, T* p) {
        return DPA_SetPtr(m_hdpa, i, p);
    }
    HRESULT InsertPtr(int i, T* p, int* outIndex = nullptr) {
        int result = DPA_InsertPtr(m_hdpa, i, p);
        if (outIndex)
            *outIndex = result;
        if (result == -1)
            return E_OUTOFMEMORY;
        return S_OK;
    }
    T* DeletePtr(int i) {
        return (T*)DPA_DeletePtr(m_hdpa, i);
    }
    BOOL DeleteAllPtrs() {
        return DPA_DeleteAllPtrs(m_hdpa);
    }

    void EnumCallback(EnumCallbackType pfnCB, void* pData = nullptr) {
        DPA_EnumCallback(m_hdpa, (PFNDPAENUMCALLBACK)pfnCB, pData);
    }
    void DestroyCallback(EnumCallbackType pfnCB, void* pData = nullptr) {
        if (m_hdpa) {
            DPA_DestroyCallback(m_hdpa, (PFNDPAENUMCALLBACK)pfnCB, pData);
            m_hdpa = nullptr;
        }
    }

    int GetPtrCount() const {
        return m_hdpa ? DPA_GetPtrCount(m_hdpa) : 0;
    }
    void SetPtrCount(int cItems) {
        DPA_SetPtrCount(m_hdpa, cItems);
    }
    T** GetPtrPtr() const {
        return (T**)DPA_GetPtrPtr(m_hdpa);
    }
    T*& FastGetPtr(int i) const {
        return (T*&)DPA_FastGetPtr(m_hdpa, i);
    }
    HRESULT AppendPtr(T* p, int* outIndex = nullptr) {
        int result = DPA_AppendPtr(m_hdpa, p);
        if (outIndex)
            *outIndex = result;
        if (result == -1)
            return E_OUTOFMEMORY;
        return S_OK;
    }

    ULONGLONG GetSize() {
        return DPA_GetSize(m_hdpa);
    }

    HRESULT LoadStream(PFNDPASTREAM pfn, IStream* pstream, void* pvInstData) {
        return DPA_LoadStream(&m_hdpa, pfn, pstream, pvInstData);
    }
    HRESULT SaveStream(PFNDPASTREAM pfn, IStream* pstream, void* pvInstData) {
        return DPA_SaveStream(m_hdpa, pfn, pstream, pvInstData);
    }

    BOOL Sort(CompareType pfnCompare, LPARAM lParam) {
        return DPA_Sort(m_hdpa, (PFNDACOMPARE)pfnCompare, lParam);
    }
    BOOL Merge(CDPA_Base* pdpaDest, DWORD dwFlags, CompareType pfnCompare, MergeType pfnMerge, LPARAM lParam) {
        return DPA_Merge(m_hdpa, pdpaDest->m_hdpa, dwFlags, (PFNDACOMPARE)pfnCompare, (PFNDPAMERGE)pfnMerge, lParam);
    }
    int Search(T* pFind, int iStart, CompareType pfnCompare, LPARAM lParam, UINT options) {
        return DPA_Search(m_hdpa, pFind, iStart, (PFNDACOMPARE)pfnCompare, lParam, options);
    }
    BOOL SortedInsertPtr(T* pItem, int iStart, CompareType pfnCompare, LPARAM lParam, UINT options, T* pFind) {
        return DPA_SortedInsertPtr(m_hdpa, pFind, iStart, (PFNDACOMPARE)pfnCompare, lParam, options, pItem);
    }

private:
    static int _StandardDestroyCB(T* p, void* pData) {
        ContainerPolicy::Destroy(p);
        return 1;
    }

    HDPA m_hdpa;
};

template<typename T, typename ContainerPolicy>
class CDPA : public CDPA_Base<T, ContainerPolicy> {
public:
    CDPA(HDPA hdpa = nullptr)
        : CDPA_Base<T, ContainerPolicy>(hdpa) {
    }
};

template<typename T>
class CTContainer_PolicyUnOwned {
public:
    static void Destroy(T* p) {}
};