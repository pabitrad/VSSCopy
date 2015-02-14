#pragma once

#include "CWriterComponent.h"

using namespace std;

class CWriter
{
private:
    vector<CWriterComponent> _components;
    vector<CWriterComponent> _componentTree; 
    GUID _instanceId;
    CString _name; 
    GUID _writerId; 
    
public:
    vector<CWriterComponent>& get_Components(void)
    {
        return _components; 
    }

    GUID get_InstanceId(void)
    {
        return _instanceId; 
    }

    void set_InstanceId(GUID& value)
    {
        _instanceId = value;
    }

    CString& get_Name(void)
    {
        return _name; 
    }

    void set_Name(CString name)
    {
        _name = name; 
    }

    GUID get_WriterId(void)
    {
        return _writerId;
    }

    void set_WriterId(GUID& value)
    {
        _writerId = value; 
    }

    void ComputeComponentTree(void)
    {
        for (unsigned int iComponent = 0; iComponent < _components.size(); ++iComponent)
        {
            CWriterComponent& current = _components[iComponent];

            for (unsigned int iComponentParent = 0; iComponentParent < _components.size(); ++iComponentParent)
            {
                if (iComponentParent == iComponent)
                {
                    continue; 
                }

                CWriterComponent& potentialParent = _components[iComponentParent]; 
                if (potentialParent.IsParentOf(current))
                {
                    current.set_Parent(&potentialParent); 
                    break; 
                }
            }
        }

    }
};