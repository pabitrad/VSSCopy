#pragma once

using namespace std;

#include "CVSSCopyException.h"
#include "Utilities.h"

class CWriterComponent
{
private:
    CString _logicalPath;
    bool _logicalPathParsed; 
    CString _name; 
    CWriterComponent* _pParent; 
//    vector<CString> _pathComponents;
    bool _selectableForBackup; 
    VSS_COMPONENT_TYPE _type; 
    int _writer; 

public:
    CWriterComponent::CWriterComponent()
    {
        _pParent = NULL; 
    }

    bool get_HasSelectableAncestor(void)
    {
        if (_pParent == NULL)
        {
            return false; 
        }

        if (_pParent->get_SelectableForBackup())
        {
            return true; 
        }

        return _pParent->get_HasSelectableAncestor(); 
    }

    CString get_LogicalPath(void)
    {
        return _logicalPath; 
    }

    void set_LogicalPath(CString logicalPath)
    {
        _logicalPath = logicalPath; 
        _logicalPathParsed = false; 
    }

    CString get_Name(void)
    {
        return _name; 
    }

    void set_Name(CString value)
    {
        _name = value; 
    }

    CWriterComponent* get_Parent(void)
    {
        return _pParent; 
    }

    void set_Parent(CWriterComponent* value)
    {
        _pParent = value; 
    }

    bool get_SelectableForBackup(void)
    {
        return _selectableForBackup; 
    }

    void set_SelectableForBackup(bool selectableForBackup)
    {
        _selectableForBackup = selectableForBackup; 
    }

    VSS_COMPONENT_TYPE get_Type(void)
    {
        return _type; 
    }

    void set_Type(VSS_COMPONENT_TYPE value)
    {
        _type = value; 
    }

    int get_Writer(void)
    {
        return _writer; 
    }

    void set_Writer(int writer)
    {
        _writer = writer; 
    }
/*
    bool IsAncestorOf(CWriterComponent& potentialDescendant)
    {
        ParseLogicalPath(); 
        potentialDescendant.ParseLogicalPath(); 
        
        if (_pathComponents.size() >= potentialDescendant._pathComponents.size())
        {
            return false; 
        }

        for (unsigned int iPathComponent = 0; 
            iPathComponent < _pathComponents.size(); 
            ++iPathComponent)
        {
            if (potentialDescendant._pathComponents[iPathComponent].Compare(_pathComponents[iPathComponent]) != 0)
            {
                return false; 
            }
        }

        return true; 
    }
*/
    bool IsParentOf(CWriterComponent& potentialParent)
    {
        // The other component is our parent if our logical path is equal to 
        // their logical path plus their name. 
        CString pathToCompare = potentialParent.get_LogicalPath(); 

        if (pathToCompare.GetLength() > 0)
        {
            pathToCompare.Append(TEXT("\\")); 
        }

        pathToCompare.Append(potentialParent.get_Name()); 

        return get_LogicalPath().Compare(pathToCompare) == 0; 
    }

private:
/*
    void ParseLogicalPath()
    {
        if (_logicalPathParsed)
        {
            return; 
        }

        _logicalPathParsed = true; 

        _pathComponents.clear(); 

        _pathComponents.push_back(TEXT("")); 

        if (_logicalPath.IsEmpty())
        {
            return; 
        }

        bool done = false; 
        int start = 0;
        while (!done)
        {
            CString component = _logicalPath.Tokenize(TEXT("\\"), start); 

            if (start == -1)
            {
                done = true; 
            }
            else
            {
                _pathComponents.push_back(component); 
            }
        }
    }
*/
};