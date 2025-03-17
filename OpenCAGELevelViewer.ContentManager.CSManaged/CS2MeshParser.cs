using System.Numerics;
using static CATHODE.Models;

namespace OpenCAGELevelViewer.ContentManager.CSManaged
{
	public struct Vector2<T> where T : notnull, INumber<T>
	{
		public T x;
		public T y;
		public Vector2(T x, T y)
		{
			this.x = x;
			this.y = y;
		}
	}

	public struct Vector3<T> where T : notnull, INumber<T>
	{
		public T x;
		public T y;
		public T z;

		public Vector3(T x, T y, T z)
		{
			this.x = x;
			this.y = y;
			this.z = z;
		}
	}

	public struct Vector4<T> where T : notnull, INumber<T>
	{
		public T x;
		public T y;
		public T z;
		public T w;

		public Vector4(T x, T y, T z, T w)
		{
			this.x = x;
			this.y = y;
			this.z = z;
			this.w = w;
		}

		public Vector4<T> Normalized
		{
			get
			{
				T max = new T[] { x, y, z, w }.Max();

				return new(
					x / max,
					y / max,
					z / max,
					w / max
					);
			}
		}
	}

	public class ParsedCS2Mesh
	{
		public List<UInt16> indices = [];
		public List<Vector3<float>> vertices = [];
		public List<Vector3<float>> normals = [];
		public List<Vector4<float>> tangents = [];
		public List<Vector2<float>> uv0 = [];
		public List<Vector2<float>> uv1 = [];
		public List<Vector2<float>> uv2 = [];
		public List<Vector2<float>> uv3 = [];
		public List<Vector2<float>> uv7 = [];

		public List<Vector4<byte>> boneIndex = []; //The indexes of 4 bones that affect each vertex
		public List<Vector4<float>> boneWeight = [];
	}

	public static class CathodeLibExtensions
	{
		public static ParsedCS2Mesh SubmeshToParsedMesh(CS2.Component.LOD.Submesh submesh)
		{
			ParsedCS2Mesh mesh = new();

			if (submesh == null || submesh.content.Length == 0)
				return mesh;

			using (BinaryReader reader = new BinaryReader(new MemoryStream(submesh.content)))
			{
				for (int i = 0; i < submesh.VertexFormat.Elements.Count; ++i)
				{
					if (i == submesh.VertexFormat.Elements.Count - 1)
					{
						//TODO: should probably properly verify VariableType here 
						// if (submesh.VertexFormat.Elements[i].Count != 1 || submesh.VertexFormat.Elements[i][0].VariableType != VBFE_InputType.INDICIES_U16)
						//     throw new Exception("unexpected format");

						for (int x = 0; x < submesh.IndexCount; x++)
							mesh.indices.Add(reader.ReadUInt16());

						continue;
					}

					for (int x = 0; x < submesh.VertexCount; ++x)
					{
						for (int y = 0; y < submesh.VertexFormat.Elements[i].Count; ++y)
						{
							AlienVBF.Element format = submesh.VertexFormat.Elements[i][y];
							switch (format.VariableType)
							{
								case VBFE_InputType.VECTOR3:
								{
									Vector3<float> v = new(reader.ReadSingle(), reader.ReadSingle(), reader.ReadSingle());
									switch (format.ShaderSlot)
									{
										case VBFE_InputSlot.NORMAL:
											mesh.normals.Add(v);
											break;
										case VBFE_InputSlot.TANGENT:
											mesh.tangents.Add(new Vector4<float>(( float ) v.x, ( float ) v.y, ( float ) v.z, 0));
											break;
										case VBFE_InputSlot.UV:
											//TODO: 3D UVW
											break;
									}
									;
									break;
								}
								case VBFE_InputType.INT32:
								{
									int v = reader.ReadInt32();
									switch (format.ShaderSlot)
									{
										case VBFE_InputSlot.COLOUR:
											//??
											break;
									}
									break;
								}
								case VBFE_InputType.VECTOR4_BYTE:
								{
									Vector4<byte> v = new(reader.ReadByte(), reader.ReadByte(), reader.ReadByte(), reader.ReadByte());
									switch (format.ShaderSlot)
									{
										case VBFE_InputSlot.BONE_INDICES:
											mesh.boneIndex.Add(v);
											break;
									}
									break;
								}
								case VBFE_InputType.VECTOR4_BYTE_DIV255:
								{
									Vector4<float> v = new(reader.ReadByte() / 255, reader.ReadByte() / 255, reader.ReadByte() / 255, reader.ReadByte() / 255);
									//v /= 255.0f;
									switch (format.ShaderSlot)
									{
										case VBFE_InputSlot.BONE_WEIGHTS:
											float vAggregate = (v.x + v.y + v.z + v.w);
											mesh.boneWeight.Add(new(v.x / vAggregate, v.y / vAggregate, v.z / vAggregate, v.w / vAggregate));
											break;
										case VBFE_InputSlot.UV:
											mesh.uv2.Add(new(v.x, v.y));
											mesh.uv3.Add(new(v.z, v.w));
											break;
									}
									break;
								}
								case VBFE_InputType.VECTOR2_INT16_DIV2048:
								{
									Vector2<float> v = new(reader.ReadInt16() / 2048.0f, reader.ReadInt16() / 2048.0f);
									switch (format.ShaderSlot)
									{
										case VBFE_InputSlot.UV:
											if (format.VariantIndex == 0) mesh.uv0.Add(v);
											else if (format.VariantIndex == 1)
											{
												// TODO: We can figure this out based on AlienVBFE.
												//Material->Material.Flags |= Material_HasTexCoord1;
												mesh.uv1.Add(v);
											}
											else if (format.VariantIndex == 2) mesh.uv2.Add(v);
											else if (format.VariantIndex == 3) mesh.uv3.Add(v);
											else if (format.VariantIndex == 7) mesh.uv7.Add(v);
											break;
									}
									break;
								}
								case VBFE_InputType.VECTOR4_INT16_DIVMAX:
								{
									Vector4<float> v = new(
										reader.ReadInt16() / ( float ) Int16.MaxValue * submesh.ScaleFactor,
										reader.ReadInt16() / ( float ) Int16.MaxValue * submesh.ScaleFactor,
										reader.ReadInt16() / ( float ) Int16.MaxValue * submesh.ScaleFactor,
										reader.ReadInt16() / ( float ) Int16.MaxValue * submesh.ScaleFactor
										);
									//v /= ( float ) Int16.MaxValue;
									//if (v.w != 0 && v.w != -1 && v.w != 1) throw new Exception("Unexpected vert W");
									//v *= submesh.ScaleFactor; //Account for scale
									switch (format.ShaderSlot)
									{
										case VBFE_InputSlot.VERTEX:
											mesh.vertices.Add(new(v.x, v.y, v.z));
											break;
									}
									break;
								}
								case VBFE_InputType.VECTOR4_BYTE_NORM:
								{
									Vector4<float> v = new(reader.ReadByte() / ( float ) byte.MaxValue - 0.5f, reader.ReadByte() / ( float ) byte.MaxValue - 0.5f, reader.ReadByte() / ( float ) byte.MaxValue - 0.5f, reader.ReadByte() / ( float ) byte.MaxValue - 0.5f);
									//v /= ( float ) byte.MaxValue - 0.5f;
									v = v.Normalized;
									switch (format.ShaderSlot)
									{
										case VBFE_InputSlot.NORMAL:
											mesh.normals.Add(new Vector3<float>(v.x, v.y, v.z));
											break;
										case VBFE_InputSlot.TANGENT:
											break;
										case VBFE_InputSlot.BITANGENT:
											break;
									}
									break;
								}
							}
						}
					}
					CathodeLib.Utilities.Align(reader, 16);
				}
			}
			return mesh;
		}
	}
}