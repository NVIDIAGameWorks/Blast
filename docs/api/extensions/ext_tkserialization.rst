.. _pageexttkserialization:

BlastTk Serialization (NvBlastExtTkSerialization)
-------------------------------------------------

This extension contains serializers which can be loaded into the ExtSerialization manager defined in :ref:`pageextserialization`.

To use this extension, you must also load the ExtSerialization extension and create a serialization manager as described in :ref:`pageextserialization`.

We repeat this here (again, assuming we are in the Nv::Blast namespace):

.. code-block:: text

    ExtSerialization* ser = NvBlastExtSerializationCreate();


Then, call the function NvBlastExtTkSerializerLoadSet, declared in **NvBlastExtTkSerialization.h**, passing in your TkFramework:

.. code-block:: text

    TkFramework* framework = ... // We must have created a TkFramework
    
    NvBlastExtTkSerializerLoadSet(*framework, *ser);


Now your serialization manager will have the serializers provided by this extension.  Currently only TkAsset serializers exist, with object type ID
given by

**TkObjectTypeID::Asset**


As with low-level assets, you can serialize using the serialization manager directly:

.. code-block:: text

    const TkAsset* asset = ... // Given pointer to an Nv::Blast::TkAsset
    
    void* buffer;
    uint64_t size = ser->serializeIntoBuffer(buffer, asset, TkObjectTypeID::Asset);


or use the wrapper function defined in **NvBlastExtTkSerialization.h**:

.. code-block:: text

    void* buffer;
    uint64_t size = NvBlastExtSerializationSerializeTkAssetIntoBuffer(buffer, *ser, asset);


