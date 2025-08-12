#include "ContentManager.h"

#include <chrono>
#include <iostream>
#include <msclr/gcroot.h>
#include <msclr/marshal.h>
#include <mutex>
#include <string>
#include <thread>

//#include <cliext/hash_map>
//#include <cliext/list>
//#include <cliext/vector>

#include "glm/ext/vector_int4_sized.hpp"
#include "glm/ext/vector_uint4_sized.hpp"
#include <algorithm>
#include <array>
#include <exception>
#include <functional>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iterator>
#include <map>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

using namespace std::chrono_literals;
using namespace System;

static msclr::gcroot< msclr::interop::marshal_context ^ > msclr_context = gcnew msclr::interop::marshal_context();

#define MarshalCliString(cliString) msclr_context->marshal_as<const char *>(cliString)
#define ConvertCliStringToCXXString(cliString) std::string(MarshalCliString(cliString))

std::mutex gameRootMutex;
std::string gameRoot = "";
void OpenCAGELevelViewer::AllInOne::ContentManager::setGameRoot(const std::string &gameRootPath) {
	std::lock_guard<std::mutex> lock(gameRootMutex);
	gameRoot = gameRootPath;
}
std::string OpenCAGELevelViewer::AllInOne::ContentManager::getGameRoot() {
	std::lock_guard<std::mutex> lock(gameRootMutex);
	return gameRoot;
}

std::mutex levelMutex;
std::string level = "";
void OpenCAGELevelViewer::AllInOne::ContentManager::setLevel(const std::string &_level) {
	std::lock_guard<std::mutex> lock(levelMutex);
	level = _level;
}
std::string OpenCAGELevelViewer::AllInOne::ContentManager::getLevel() {
	std::lock_guard<std::mutex> lock(levelMutex);
	return level;
}

std::mutex compositeMutex;
CATHODE::Scripting::ShortGuid compositeGuid = CATHODE::Scripting::ShortGuid::Invalid;
void OpenCAGELevelViewer::AllInOne::ContentManager::setComposite(uint32_t _compositeGuid) {
	std::lock_guard<std::mutex> lock(compositeMutex);
	auto temp = CATHODE::Scripting::ShortGuid(_compositeGuid);
	if (!temp.IsInvalid)
		compositeGuid = temp;

	//std::cout << MarshalCliString(temp.ToByteString()) << std::endl;
	//std::cout << MarshalCliString(compositeGuid.ToByteString()) << std::endl;
}
uint32_t OpenCAGELevelViewer::AllInOne::ContentManager::getComposite() {
	std::lock_guard<std::mutex> lock(compositeMutex);
	return compositeGuid.AsUInt32;
}

ref struct LevelContent {
	CATHODE::Commands ^Commands;
	CATHODE::Models ^Models;
	CATHODE::RenderableElements ^Renderables;
};

typedef glm::vec3 Vector3;
typedef struct {
	Vector3 position {};
	Vector3 rotation {};
} Transform;

typedef System::Numerics::Vector3 ManagedVector3;
ref struct ManagedTransform {
	ManagedVector3 position {};
	ManagedVector3 rotation {};

	//ManagedTransform() = default;
};

#pragma region std::hash Injections
//template<>
//struct std::hash<CATHODE::Scripting::ShortGuid> {
//	std::size_t operator()(CATHODE::Scripting::ShortGuid &guid) const noexcept {
//		return guid.GetHashCode();
//	}
//};
#pragma endregion

typedef ref struct EntityDataValue;
//typedef ref struct ManagedExtendedEntityData;
//
//typedef std::pair < ExtendedEntityData, ManagedExtendedEntityData > EntityDataValue;
//
typedef System::Collections::Generic::Dictionary < CATHODE::Scripting::ShortGuid, EntityDataValue ^ > ShortGuidEntityMap;

ref struct EntityDataValue {
	ManagedTransform ^transform {};

	CATHODE::Scripting::Internal::Entity ^entity = nullptr;

	EntityDataValue ^parent = nullptr;

	/*EntityDataValue() = default;
	EntityDataValue(const EntityDataValue ^other) = default;*/
};

ref struct ModelReferenceDataValue : public EntityDataValue {
	size_t modelReferenceId {};
};

ref struct CompositeDataValue : public EntityDataValue {
	ShortGuidEntityMap ^children = gcnew ShortGuidEntityMap(0);
	CATHODE::Scripting::Composite ^composite = nullptr;
};

static_assert(std::is_trivially_copyable<OpenCAGELevelViewer::AllInOne::ContentManager::ModelReferenceGL>::value);

msclr::gcroot < LevelContent ^ > levelContentInstance = nullptr;

// TODO: Expose these as global variables
msclr::gcroot < System::Collections::Generic::List < System::Collections::Generic::List < CATHODE::Scripting::ShortGuid > ^ > ^ > compositesById {};
std::map < uint64_t, OpenCAGELevelViewer::AllInOne::ContentManager::CMModel > models {};
std::map < uint64_t, std::vector < OpenCAGELevelViewer::AllInOne::ContentManager::ModelReferenceGL > > modelReferences {};
msclr::gcroot < CompositeDataValue ^ > entityDataRoot { };

#pragma region Model Loading and Handling
// ContentManager model

template<typename T, bool CorrectForEndianness = (sizeof(T) > 1), bool IsDataLittleEndian = true, size_t ByteSwapDivisor = sizeof(T)>
static T popTFromArray(size_t &rollingIndex, const std::vector < unsigned char > &array) {
	static_assert(std::is_trivially_copyable<T>::value);
	static_assert((!CorrectForEndianness) || (sizeof(T) > 1));

	std::array < unsigned char, sizeof(T) > tArrayInstance {};
	// Check that std::array is just a raw C-style array under the hood.
	// If it is, then bit_cast can work on it just fine, else UB.
	static_assert(sizeof(std::array < unsigned char, sizeof(T) >) == sizeof(T));

	//std::array < uint16_t, 2 > test {};
	//sizeof(test);

	for (size_t i = 0; i < sizeof(T); i++) {
		tArrayInstance[i] = array[rollingIndex + i];
	}

	if constexpr (CorrectForEndianness && ((IsDataLittleEndian && std::endian::native == std::endian::big) || (!IsDataLittleEndian && std::endian::native == std::endian::little))) {
		static_assert(sizeof(T) % ByteSwapDivisor == 0);

		std::array < unsigned char, ByteSwapDivisor > tempTArrayInstance;

		for (size_t i = 0; i < sizeof(T); i += ByteSwapDivisor) {
			tempTArrayInstance = {};

			for (size_t x = 0; x < ByteSwapDivisor; x++) {
				tempTArrayInstance[x] = tArrayInstance[i + x];
			}

			for (size_t x = 0; x < ByteSwapDivisor; x++) {
				tArrayInstance[i + x] = tempTArrayInstance[(ByteSwapDivisor - 1) - x];
			}
		}
	}

	rollingIndex += sizeof(T);

	return std::bit_cast< T >(tArrayInstance);
}

static std::optional < OpenCAGELevelViewer::AllInOne::ContentManager::CMModel > getCMMModel(int entryIndex) {
	if (!models.contains(entryIndex)) {
		CATHODE::Models::CS2::Component::LOD::Submesh ^submesh = levelContentInstance->Models->GetAtWriteIndex(entryIndex);
		CATHODE::Models::CS2::Component::LOD ^lod = levelContentInstance->Models->FindModelLODForSubmesh(submesh);
		CATHODE::Models::CS2 ^mesh = levelContentInstance->Models->FindModelForSubmesh(submesh);

		if (submesh == nullptr)
			return std::nullopt;

		OpenCAGELevelViewer::AllInOne::ContentManager::CMModel model {};

		model.meshName = ConvertCliStringToCXXString(mesh->Name);
		model.modelId = entryIndex;

		model.vertices.resize(submesh->VertexCount);
		model.elements.resize(submesh->IndexCount);

		// The following code only cares about the vertices and elements.
		// Not worrying about the extra stuff until I can get level loading and rendering
		{
			std::vector < unsigned char > submeshContent = std::vector < unsigned char >(submesh->content->Length);
			System::Runtime::InteropServices::Marshal::Copy(submesh->content, 0, IntPtr(submeshContent.data()), submesh->content->Length);

			size_t rollingIndex = 0;
			
			System::Collections::Generic::List < System::Collections::Generic::List < CATHODE::Models::AlienVBF::Element ^ > ^ > ^formats = submesh->VertexFormat->Elements;

			for (size_t i = 0; i < formats->Count; i++) {
				//std::cout << "Processing seg " << i + 1 << "/" << formats->Count << std::endl << std::flush;

				System::Collections::Generic::List < CATHODE::Models::AlienVBF::Element ^ > ^elements = formats[i];
				
				bool inIndexMode = i == formats->Count - 1;
				size_t xIterations = inIndexMode ? submesh->IndexCount : submesh->VertexCount;

				//std::cout << (inIndexMode ? "index mode" : "vertex mode") << std::endl << std::flush;

				for (size_t x = 0; x < xIterations; x++) {
					//std::cout << "Processing x " << x + 1 << "/" << xIterations << std::endl << std::flush;

					for (size_t y = 0; y < elements->Count; y++) {
						//std::cout << "Processing element " << y + 1 << "/" << elements->Count << std::endl << std::flush;

						CATHODE::Models::AlienVBF::Element ^element = elements[y];

						if (inIndexMode)
							if (element->VariableType == CATHODE::Models::VBFE_InputType::INDICIES_U16)
								model.elements[x] = popTFromArray< uint16_t >(rollingIndex, submeshContent);
							else
								std::cout << "Refusing to process non-index vertex format at end of submesh." << std::endl << std::flush;
						else {
							switch (element->VariableType) {
								[[unlikely]] case CATHODE::Models::VBFE_InputType::INDICIES_U16:
									std::cout << "Refusing to process index vertex format." << std::endl << std::flush;
									break;

								case CATHODE::Models::VBFE_InputType::VECTOR3:
									{
										glm::fvec3 v = popTFromArray< glm::fvec3, true, true, 4 >(rollingIndex, submeshContent);

										// TODO: Normal, tanget, UV

										break;
									}

								case CATHODE::Models::VBFE_InputType::INT32:
									{
										int32_t v = popTFromArray< int32_t >(rollingIndex, submeshContent);

										switch (element->ShaderSlot) {
											case CATHODE::Models::VBFE_InputSlot::COLOUR:
												model.vertices[x].col = std::bit_cast< glm::u8vec4 >(v);
												break;
											default:
												std::cout << "Discarding possibly invalid COL vertex format" << std::endl << std::flush;
												break;
										}
										break;
									}

								case CATHODE::Models::VBFE_InputType::VECTOR4_BYTE:
									{
										glm::u8vec4 v = popTFromArray< glm::u8vec4, false >(rollingIndex, submeshContent);

										break;
									}

								case CATHODE::Models::VBFE_InputType::VECTOR4_BYTE_DIV255:
									{
										glm::fvec4 v = popTFromArray< glm::u8vec4, false >(rollingIndex, submeshContent);
										v /= 255.0f;

										// TODO: Bone weights & UV

										break;
									}

								case CATHODE::Models::VBFE_InputType::VECTOR4_BYTE_NORM:
									{
										glm::fvec4 v = popTFromArray< glm::u8vec4, false >(rollingIndex, submeshContent);
										v /= static_cast < float >(std::numeric_limits < uint8_t >::max()) - 0.5f;
										v = glm::normalize(v);

										// TODO: Normal, tanget, bitanget

										break;
									}

								case CATHODE::Models::VBFE_InputType::VECTOR4_INT16_DIVMAX:
									{
										//glm::i16vec4 vT = popTFromArray< glm::i16vec4, true, true, 2 >(rollingIndex, submeshContent);
										glm::fvec4 v = popTFromArray< glm::i16vec4, true, true, 2 >(rollingIndex, submeshContent);
										v /= static_cast < float >(std::numeric_limits < int16_t >::max());

										if (v.w != 0 && v.w != -1 && v.w != 1)
											throw std::exception("Unexpected vert W");

										v *= submesh->ScaleFactor;

										switch (element->ShaderSlot) {
											case CATHODE::Models::VBFE_InputSlot::VERTEX:
												{
													model.vertices[x].pos = glm::fvec3(v.x, v.y, v.z);
													break;
												}
										}

										break;
									}

								case CATHODE::Models::VBFE_InputType::VECTOR2_INT16_DIV2048:
									{
										glm::fvec2 v = popTFromArray< glm::i16vec2, true, true, 2 >(rollingIndex, submeshContent);
										v /= 2048.0f;

										// TODO: UV

										break;
									}
							}
						}
					}
				}
			}
			
		#if 0
			for (size_t i = 0; i < submesh->VertexFormat->Elements->Count; ++i) {
				if (i == submesh->VertexFormat->Elements->Count - 1) {
					if (submesh->VertexFormat->Elements[i]->Count != 1 || submesh->VertexFormat->Elements[i][0]->VariableType != CATHODE::Models::VBFE_InputType::INDICIES_U16)
						throw new std::exception("Unexpected format");

					for (size_t x = 0; x < submesh->IndexCount; x++) {
						uint16_t test = popTFromArray< uint16_t >(rollingIndex, submeshContent);
						model.elements.push_back(test);
					}

					continue;
				}

				for (size_t x = 0; x < submesh->VertexCount; ++x) {
					for (size_t y = 0; y < submesh->VertexFormat->Elements[i]->Count; ++y) {
						CATHODE::Models::AlienVBF::Element ^format = submesh->VertexFormat->Elements[i][y];
						switch (format->VariableType) {
							case CATHODE::Models::VBFE_InputType::VECTOR3:
								{
									glm::fvec3 v = popTFromArray< glm::fvec3, true, true, 4 >(rollingIndex, submeshContent);

									std::cout << "test" << std::endl;

									// TODO: Normal, tanget, UV

									break;
								}
							case CATHODE::Models::VBFE_InputType::INT32:
								{
									int32_t v = popTFromArray< int32_t >(rollingIndex, submeshContent);

									// TODO: Colour

									break;
								}
							case CATHODE::Models::VBFE_InputType::VECTOR4_BYTE:
								{
									glm::u8vec4 v = popTFromArray< glm::u8vec4, false >(rollingIndex, submeshContent);

									//{
									//	uint8_t x = 0;
									//	uint8_t y = 0;
									//	uint8_t z = 0;
									//	uint8_t w = 0;

									//	x = popTFromArray< uint8_t >(rollingIndex, submeshContent);
									//	y = popTFromArray< uint8_t >(rollingIndex, submeshContent);
									//	z = popTFromArray< uint8_t >(rollingIndex, submeshContent);
									//	w = popTFromArray< uint8_t >(rollingIndex, submeshContent);

									//	v = glm::u8vec4(x, y, z, w);
									//}

									// TODO: Bone indices

									break;
								}
							case CATHODE::Models::VBFE_InputType::VECTOR4_BYTE_DIV255:
								{
									glm::fvec4 v = popTFromArray< glm::u8vec4, false >(rollingIndex, submeshContent);
									v /= 255.0f;
									
									// TODO: Bone weights, UV

									break;
								}
							case CATHODE::Models::VBFE_InputType::VECTOR2_INT16_DIV2048:
								{
									glm::fvec2 v = popTFromArray< glm::i16vec2, true, true, 2 >(rollingIndex, submeshContent);
									v /= 2048.0f;

									// TODO: UV

									break;
								}
							case CATHODE::Models::VBFE_InputType::VECTOR4_INT16_DIVMAX:
								{									
									glm::fvec4 v = popTFromArray< glm::i16vec4, true, true, 2 >(rollingIndex, submeshContent);

									v /= static_cast < float >(std::numeric_limits < int16_t >::max());

									if (v.w != 0 && v.w != -1 && v.w != 0)
										throw std::exception("Unexpected vert W");

									v *= submesh->ScaleFactor;

									switch (format->ShaderSlot) {
										case CATHODE::Models::VBFE_InputSlot::VERTEX:
											model.vertices.push_back(glm::fvec3(v.x, v.y, v.z));
											break;
									}

									break;
								}
							case CATHODE::Models::VBFE_InputType::VECTOR4_BYTE_NORM:
								{
									glm::fvec4 v = popTFromArray< glm::u8vec4, false >(rollingIndex, submeshContent);;
									v /= static_cast < float >(std::numeric_limits < uint8_t >::max()) - 0.5f;
									v = glm::normalize(v);

									// TODO: Normal, tanget, bitanget

									break;
								}
						}
					}
				}
				constexpr size_t rollingIndexModulo = 16;
				rollingIndex += rollingIndex - (rollingIndex % rollingIndexModulo);
			}
		#endif
		}

		//OpenCAGELevelViewer::AllInOne::CSManaged::getManagedCMModel(submesh);

		// TODO: Interleave to create a vertex buffer object

		models[entryIndex] = model;
		return model;
	} else {
		return models[entryIndex];
	}
}

//static std::optional < OpenCAGELevelViewer::AllInOne::ContentManager::CMModel & > antiCopyGetCmModel {
//
//}
#pragma endregion

static EntityDataValue ^getEntityFromEntityPath(System::Collections::Generic::List < CATHODE::Scripting::ShortGuid > ^entityPath) {
	CompositeDataValue ^current = entityDataRoot.operator CompositeDataValue ^ ();
	array < CATHODE::Scripting::ShortGuid > ^entityPathAsArray = entityPath->ToArray();

	for (size_t i = 0; i < entityPath->Count; i++) {
		CATHODE::Scripting::ShortGuid shortGuid = entityPathAsArray[i];
		EntityDataValue ^entityDataValue = current->children[shortGuid];

		if (i == entityPath->Count - 1)
			return entityDataValue;
		else
			current = dynamic_cast < CompositeDataValue ^ >(entityDataValue);
	}
}

static System::Collections::Generic::List < CATHODE::Scripting::ShortGuid > ^getEntityPathFromEntity(EntityDataValue ^entityDataValue) {
	System::Collections::Generic::List < CATHODE::Scripting::ShortGuid > ^entityPath = gcnew System::Collections::Generic::List < CATHODE::Scripting::ShortGuid >(0);

	EntityDataValue ^current = entityDataValue;

	while (true) {
		if (current->entity == nullptr)
			break;

		CATHODE::Scripting::ShortGuid newShortGuid = current->entity->shortGUID;
		entityPath->Insert(0, newShortGuid);

		current = current->parent;
	}

	return entityPath;
}

static bool siftEntity(CATHODE::Scripting::Internal::Entity ^source, int _) {
	return source->variant != CATHODE::Scripting::EntityVariant::VARIABLE;
}

static uint8_t sortEntity(CATHODE::Scripting::Internal::Entity ^source) {
	// Internally, EntityVariant has some good values already - most notably, the Function & Variable entity is lower than the Alias & Proxy entities.
	// However, I'll still use a switch statement to make it more reliable in case of changes.

	switch (source->variant) {
		case CATHODE::Scripting::EntityVariant::FUNCTION:
			return 0;
		case CATHODE::Scripting::EntityVariant::VARIABLE:
			return 1;
		case CATHODE::Scripting::EntityVariant::ALIAS:
			return 2;
		case CATHODE::Scripting::EntityVariant::PROXY:
			return 3;
	}
}

static ManagedTransform ^getManagedTransformFromEntity(CATHODE::Scripting::FunctionEntity ^entity) {
	ManagedTransform ^transform = gcnew ManagedTransform();
	CATHODE::Scripting::Parameter ^positionParam = entity->GetParameter("position");
	if (positionParam != nullptr && positionParam->content != nullptr) {
		switch (positionParam->content->dataType) {
			case CATHODE::Scripting::DataType::TRANSFORM: {
				CATHODE::Scripting::cTransform ^transformContent = dynamic_cast< CATHODE::Scripting::cTransform ^ >(positionParam->content);
				if (transformContent != nullptr) {
					transform->position = System::Numerics::Vector3(transformContent->position.X, transformContent->position.Y, transformContent->position.Z);
					transform->rotation = System::Numerics::Vector3(transformContent->rotation.X, transformContent->rotation.Y, transformContent->rotation.Z);
				}
				break;
			}
			default:
				break;
		}
	}
	return transform;
}

static void CascadeMain(CompositeDataValue ^composite);

static void CascadeEntity(CompositeDataValue ^parent, CATHODE::Scripting::Internal::Entity ^entity) {
	switch (entity->variant) {
		case CATHODE::Scripting::EntityVariant::FUNCTION: {
			CATHODE::Scripting::FunctionEntity ^functionEntity = dynamic_cast< CATHODE::Scripting::FunctionEntity ^ >(entity);
			if (functionEntity->function.IsFunctionType) {
				// function
				switch (functionEntity->function.AsFunctionType) {
					case CATHODE::Scripting::FunctionType::ModelReference:
						{
							ModelReferenceDataValue ^modelReferenceDataValue = gcnew ModelReferenceDataValue();
							modelReferenceDataValue->entity = functionEntity;
							modelReferenceDataValue->transform = getManagedTransformFromEntity(functionEntity);

							modelReferenceDataValue->parent = parent;
							parent->children->Add(modelReferenceDataValue->entity->shortGUID, modelReferenceDataValue);

							CATHODE::Scripting::Parameter ^resourceParameter = functionEntity->GetParameter("resource");

							System::Collections::Generic::List < System::Collections::Generic::List < CATHODE::Scripting::ShortGuid > ^ > ^tempCompById = compositesById;

							modelReferenceDataValue->modelReferenceId = compositesById->Count;

							if (!compositesById->Contains(getEntityPathFromEntity(modelReferenceDataValue)))
								compositesById->Add(getEntityPathFromEntity(modelReferenceDataValue));

							if (resourceParameter != nullptr && resourceParameter->content != nullptr && resourceParameter->content->dataType == CATHODE::Scripting::DataType::RESOURCE) {
								CATHODE::Scripting::cResource ^resource = dynamic_cast< CATHODE::Scripting::cResource ^ >(resourceParameter->content);
								CATHODE::Scripting::ResourceReference ^resourceRef = resource->GetResource(CATHODE::Scripting::ResourceType::RENDERABLE_INSTANCE);

								//int viableResourceRefs = 0;

								if (resourceRef != nullptr)
									for (size_t i = 0; i < resourceRef->count; i++) {
										CATHODE::RenderableElements::Element ^renderableElement = levelContentInstance->Renderables->Entries[resourceRef->index + i];

										std::optional < OpenCAGELevelViewer::AllInOne::ContentManager::CMModel > cmModel = getCMMModel(renderableElement->ModelIndex);

										if (cmModel.has_value()) {
											{
												OpenCAGELevelViewer::AllInOne::ContentManager::ModelReferenceGL modelReferenceGl;
												modelReferenceGl.instanceId = modelReferenceDataValue->modelReferenceId;

												if (!modelReferences.contains(cmModel->modelId))
													modelReferences[cmModel->modelId] = {};

												modelReferences[cmModel->modelId].push_back(modelReferenceGl);

												//__debugbreak();
											}

											//viableResourceRefs++;
										}
									}
							}
							break;
						}
				}
			} else {
				// composite
				CompositeDataValue ^compositeDataValue = gcnew CompositeDataValue();
				compositeDataValue->entity = functionEntity;
				compositeDataValue->composite = levelContentInstance->Commands->GetComposite(functionEntity->function);
				compositeDataValue->transform = getManagedTransformFromEntity(functionEntity);

				compositeDataValue->parent = parent;
				parent->children->Add(compositeDataValue->entity->shortGUID, compositeDataValue);

				CascadeMain(compositeDataValue);
			}
			break;
		}
	}
}

static void CascadeMain(CompositeDataValue ^composite) {
	if (composite == nullptr || composite->composite == nullptr)
		return;

	System::Collections::Generic::IList < CATHODE::Scripting::Internal::Entity ^ > ^entities = composite->composite->GetEntities();

	/*entities = System::Linq::Enumerable::ToList(System::Linq::Enumerable::OrderBy(entities, gcnew Func<CATHODE::Scripting::Internal::Entity ^, int>([](CATHODE::Scripting::Internal::Entity ^e) {
		return e->shortGUID;
	})));*/

	System::Collections::Generic::IEnumerable < CATHODE::Scripting::Internal::Entity ^ > ^entitiesEnumerable =
		System::Linq::Enumerable::OrderBy(entities, gcnew System::Func< CATHODE::Scripting::Internal::Entity ^, uint8_t >(sortEntity));

	entitiesEnumerable =
		System::Linq::Enumerable::Where<CATHODE::Scripting::Internal::Entity ^>(
			entitiesEnumerable,
			gcnew System::Func< CATHODE::Scripting::Internal::Entity ^, int, bool >(siftEntity)
		);

	for each (CATHODE::Scripting::Internal::Entity ^entity in entities) {
		CascadeEntity(composite, entity);
	}
	//System::Type ^type = entities->GetType();
	//std::cout << MarshalCliString(type->AssemblyQualifiedName) << std::endl;
	//std::cout << "stop" << std::endl;
}

static void resetGlobalVariables() {
	compositesById = gcnew System::Collections::Generic::List < System::Collections::Generic::List < CATHODE::Scripting::ShortGuid > ^ >(0);
	models = {};
	modelReferences = {};

	{
		GC::Collect();
		GC::WaitForPendingFinalizers();
		GC::Collect();
		GC::WaitForPendingFinalizers();
	}
}

static void updatePositionMatrixes() {
	for (size_t i = 0; i < compositesById->Count; i++) {
		System::Collections::Generic::List < CATHODE::Scripting::ShortGuid > ^entityShortGuidPath = compositesById->ToArray()[i];

		ModelReferenceDataValue ^modelReference = nullptr;

		glm::vec3 pos {};
		glm::vec3 rot {};

		{
			CompositeDataValue ^current = entityDataRoot.operator CompositeDataValue ^ ();
			array < CATHODE::Scripting::ShortGuid > ^entityPathAsArray = entityShortGuidPath->ToArray();

			for (size_t i = 0; i < entityPathAsArray->Length; i++) {
				CATHODE::Scripting::ShortGuid shortGuid = entityPathAsArray[i];
				EntityDataValue ^entityDataValue = current->children[shortGuid];

				pos += glm::vec3(entityDataValue->transform->position.X, entityDataValue->transform->position.Y, entityDataValue->transform->position.Z);
				rot += glm::vec3(entityDataValue->transform->rotation.X, entityDataValue->transform->rotation.Y, entityDataValue->transform->rotation.Z);

				if (i == entityPathAsArray->Length - 1)
					modelReference = dynamic_cast < ModelReferenceDataValue ^ >(entityDataValue);
				else
					current = dynamic_cast < CompositeDataValue ^ >(entityDataValue);
			}
		}

		assert(modelReference != nullptr);
		[[assume(modelReference != nullptr)]];

		glm::mat4 newMat4 = glm::mat4(1);

		newMat4 = glm::translate(newMat4, pos);
		
		// prevents NaN mat4
		if (rot != glm::vec3(0.0f))
			newMat4 = glm::rotate(newMat4, glm::radians(1.0f), rot);

		//size_t modelReferenceId = modelReference->modelReferenceId;
		
		//std::vector < OpenCAGELevelViewer::AllInOne::ContentManager::ModelReferenceGL > &currentModelReferences = modelReferences[modelReferenceId];

		for (OpenCAGELevelViewer::AllInOne::ContentManager::ModelReferenceGL &currentModelRefernceGl : modelReferences[modelReference->modelReferenceId]) {
			//newMat4[0]
			//if (newMat4 != glm::mat4(1))
			currentModelRefernceGl.worldMatrix = newMat4;
		}

		//modelReference->modelReferenceId
	}
}

void OpenCAGELevelViewer::AllInOne::ContentManager::threadMain(std::atomic_flag &suspendFlag) {
	std::string currentGameRoot = "";
	std::string currentLevel = "";
	CATHODE::Scripting::ShortGuid currentCompositeGuid = CATHODE::Scripting::ShortGuid::Invalid;

	bool gameRootDirty = false;
	bool levelDirty = false;
	bool compositeDirty = false;

	while (suspendFlag.test()) {
		{
			std::lock_guard<std::mutex> lock(gameRootMutex);
			if (currentGameRoot != gameRoot) {
				currentGameRoot = gameRoot;
				gameRootDirty = true;
			}
		}
		{
			std::lock_guard<std::mutex> lock(levelMutex);
			if (currentLevel != level) {
				currentLevel = level;
				levelDirty = true;
			}
		}
		{
			std::lock_guard<std::mutex> lock(compositeMutex);
			if (currentCompositeGuid != compositeGuid) {
				//std::cout << MarshalCliString(compositeGuid.ToByteString()) << std::endl;
				currentCompositeGuid = compositeGuid;
				//std::cout << MarshalCliString(currentCompositeGuid.ToByteString()) << std::endl;
				compositeDirty = true;
			}
		}

		if (gameRootDirty || levelDirty) {
			gameRootDirty = false;
			levelDirty = false;

			if (levelContentInstance.operator LevelContent ^ () != nullptr) {
				delete levelContentInstance;
				levelContentInstance = nullptr;
			}

			resetGlobalVariables();

			if (currentGameRoot.empty() || currentLevel.empty())
				continue;

			levelContentInstance = gcnew LevelContent();

			std::string levelPath = currentGameRoot + "/DATA/ENV/PRODUCTION/" + currentLevel + "/";

			levelContentInstance->Commands = gcnew CATHODE::Commands(gcnew String((levelPath + "WORLD/COMMANDS.PAK").c_str()));
			levelContentInstance->Models = gcnew CATHODE::Models(gcnew String((levelPath + "RENDERABLE/LEVEL_MODELS.PAK").c_str()));
			levelContentInstance->Renderables = gcnew CATHODE::RenderableElements(gcnew String((levelPath + "WORLD/REDS.BIN").c_str()));
		}

		//LevelContent ^test = levelContentInstance.operator LevelContent ^ ();
		
		//std::cout << &test << std::endl;

		if (compositeDirty && !compositeGuid.IsInvalid && levelContentInstance.operator LevelContent ^() != nullptr) {
			compositeDirty = false;

			resetGlobalVariables();

			//std::cout << MarshalCliString(currentCompositeGuid.ToByteString()) << std::endl;

			entityDataRoot = gcnew CompositeDataValue { };

			{
				GC::Collect();
				GC::WaitForPendingFinalizers();
				GC::Collect();
				GC::WaitForPendingFinalizers();
			}

			entityDataRoot->composite = levelContentInstance->Commands->GetComposite(compositeGuid);

			std::cout << "Cascade" << std::endl;

			CascadeMain(entityDataRoot);
			updatePositionMatrixes();

			{
				CompositeDataValue ^dataValue = entityDataRoot;

				std::cout << "professional do nothing" << std::endl;
			}

			std::cout << "test" << std::endl;

			//CATHODE::Scripting::Composite ^currentComposite = levelContentInstance->Commands->GetComposite(currentCompositeGuid);

			//std::cout << MarshalCliString(currentComposite->name) << std::endl;
		} else if (compositeDirty)
			if (levelContentInstance.operator LevelContent ^ () == nullptr)
				std::cout << "Waiting for levelContent." << std::endl;
			else if (compositeGuid.IsInvalid)
				std::cout << "Composite GUID is invalid." << std::endl;

		std::this_thread::sleep_for(100ms);
	}

	//SDL_GLContext glContext = SDL_GL_CreateContext(sdlWindow);

	//glbinding::useContext(1);
	//glbinding::initialize(1, reinterpret_cast< glbinding::ProcAddress(*)(const char *) >(SDL_GL_GetProcAddress), true);

	//glbinding::releaseContext();

	//SDL_GL_DestroyContext(glContext);
}