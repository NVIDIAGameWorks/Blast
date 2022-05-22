.. _pageextpxserialization:

ExtPhysX Serialization (NvBlastExtPxSerialization)
--------------------------------------------------

This extension contains serializers which can be loaded into the ExtSerialization manager defined in :ref:`pageextserialization`.

To use this extension, you must also load the ExtSerialization extension and create a serialization manager as described in :ref:`pageextserialization`.

We repeat this here (again, assuming we are in the Nv::Blast namespace):

.. code-block:: text

    ExtSerialization* ser = NvBlastExtSerializationCreate();


Then, call the function NvBlastExtPxSerializerLoadSet, declared in **NvBlastExtPxSerialization.h**, passing in your TkFramework (required by ExtPhysX), along with
your physx::PxPhysics and physx::PxCooking pointers:

.. code-block:: text

    TkFramework* framework = ... // We must have created a TkFramework
    physx::PxPhysics* physics = ... // and PxPhysics
    physx::PxCooking* cooking = ... // and PxCooking
    
    NvBlastExtPxSerializerLoadSet(*framework, *physics, *cooking *ser);


Now your serialization manager will have the serializers provided by this extension.  Currently only ExtPxAsset serializers exist, with object type ID
given by

**ExtPxObjectTypeID::Asset**


As with low-level assets, you can serialize using the serialization manager directly:

.. code-block:: text

    const ExtPxAsset* asset = ... // Given pointer to an Nv::Blast::ExtPxAsset
    
    void* buffer;
    uint64_t size = ser->serializeIntoBuffer(buffer, asset, ExtPxObjectTypeID::Asset);


or use the wrapper function defined in **NvBlastExtPxSerialization.h**:

.. code-block:: text

    void* buffer;
    uint64_t size = NvBlastExtSerializationSerializeExtPxAssetIntoBuffer(buffer, *ser, asset);


