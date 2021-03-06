// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

#include <vector>

#include "parquet/file/metadata.h"
#include "parquet/schema/converter.h"
#include "parquet/thrift/util.h"

namespace parquet {

// MetaData Accessor
// ColumnChunk metadata
class ColumnChunkMetaData::ColumnChunkMetaDataImpl {
 public:
  explicit ColumnChunkMetaDataImpl(const format::ColumnChunk* column) : column_(column) {
    const format::ColumnMetaData& meta_data = column->meta_data;
    for (auto encoding : meta_data.encodings) {
      encodings_.push_back(FromThrift(encoding));
    }
    if (meta_data.__isset.statistics) {
      stats_.null_count = meta_data.statistics.null_count;
      stats_.distinct_count = meta_data.statistics.distinct_count;
      stats_.max = &meta_data.statistics.max;
      stats_.min = &meta_data.statistics.min;
    }
  }
  ~ColumnChunkMetaDataImpl() {}

  // column chunk
  inline int64_t file_offset() const { return column_->file_offset; }
  inline const std::string& file_path() const { return column_->file_path; }

  // column metadata
  inline Type::type type() { return FromThrift(column_->meta_data.type); }

  inline int64_t num_values() const { return column_->meta_data.num_values; }

  std::shared_ptr<schema::ColumnPath> path_in_schema() {
    return std::make_shared<schema::ColumnPath>(column_->meta_data.path_in_schema);
  }

  inline bool is_stats_set() const { return column_->meta_data.__isset.statistics; }

  inline const ColumnStatistics& statistics() const { return stats_; }

  inline Compression::type compression() const {
    return FromThrift(column_->meta_data.codec);
  }

  const std::vector<Encoding::type>& encodings() const { return encodings_; }

  inline int64_t has_dictionary_page() const {
    return column_->meta_data.__isset.dictionary_page_offset;
  }

  inline int64_t dictionary_page_offset() const {
    return column_->meta_data.dictionary_page_offset;
  }

  inline int64_t data_page_offset() const { return column_->meta_data.data_page_offset; }

  inline int64_t index_page_offset() const {
    return column_->meta_data.index_page_offset;
  }

  inline int64_t total_compressed_size() const {
    return column_->meta_data.total_compressed_size;
  }

  inline int64_t total_uncompressed_size() const {
    return column_->meta_data.total_uncompressed_size;
  }

 private:
  ColumnStatistics stats_;
  std::vector<Encoding::type> encodings_;
  const format::ColumnChunk* column_;
};

std::unique_ptr<ColumnChunkMetaData> ColumnChunkMetaData::Make(const uint8_t* metadata) {
  return std::unique_ptr<ColumnChunkMetaData>(new ColumnChunkMetaData(metadata));
}

ColumnChunkMetaData::ColumnChunkMetaData(const uint8_t* metadata)
    : impl_{std::unique_ptr<ColumnChunkMetaDataImpl>(new ColumnChunkMetaDataImpl(
          reinterpret_cast<const format::ColumnChunk*>(metadata)))} {}
ColumnChunkMetaData::~ColumnChunkMetaData() {}

// column chunk
int64_t ColumnChunkMetaData::file_offset() const {
  return impl_->file_offset();
}

const std::string& ColumnChunkMetaData::file_path() const {
  return impl_->file_path();
}

// column metadata
Type::type ColumnChunkMetaData::type() const {
  return impl_->type();
}

int64_t ColumnChunkMetaData::num_values() const {
  return impl_->num_values();
}

std::shared_ptr<schema::ColumnPath> ColumnChunkMetaData::path_in_schema() const {
  return impl_->path_in_schema();
}

const ColumnStatistics& ColumnChunkMetaData::statistics() const {
  return impl_->statistics();
}

bool ColumnChunkMetaData::is_stats_set() const {
  return impl_->is_stats_set();
}

int64_t ColumnChunkMetaData::has_dictionary_page() const {
  return impl_->has_dictionary_page();
}

int64_t ColumnChunkMetaData::dictionary_page_offset() const {
  return impl_->dictionary_page_offset();
}

int64_t ColumnChunkMetaData::data_page_offset() const {
  return impl_->data_page_offset();
}

int64_t ColumnChunkMetaData::index_page_offset() const {
  return impl_->index_page_offset();
}

Compression::type ColumnChunkMetaData::compression() const {
  return impl_->compression();
}

const std::vector<Encoding::type>& ColumnChunkMetaData::encodings() const {
  return impl_->encodings();
}

int64_t ColumnChunkMetaData::total_uncompressed_size() const {
  return impl_->total_uncompressed_size();
}

int64_t ColumnChunkMetaData::total_compressed_size() const {
  return impl_->total_compressed_size();
}

// row-group metadata
class RowGroupMetaData::RowGroupMetaDataImpl {
 public:
  explicit RowGroupMetaDataImpl(
      const format::RowGroup* row_group, const SchemaDescriptor* schema)
      : row_group_(row_group), schema_(schema) {}
  ~RowGroupMetaDataImpl() {}

  inline int num_columns() const { return row_group_->columns.size(); }

  inline int64_t num_rows() const { return row_group_->num_rows; }

  inline int64_t total_byte_size() const { return row_group_->total_byte_size; }

  inline const SchemaDescriptor* schema() const { return schema_; }

  std::unique_ptr<ColumnChunkMetaData> ColumnChunk(int i) {
    if (!(i < num_columns())) {
      std::stringstream ss;
      ss << "The file only has " << num_columns()
         << " columns, requested metadata for column: " << i;
      throw ParquetException(ss.str());
    }
    return ColumnChunkMetaData::Make(
        reinterpret_cast<const uint8_t*>(&row_group_->columns[i]));
  }

 private:
  const format::RowGroup* row_group_;
  const SchemaDescriptor* schema_;
};

std::unique_ptr<RowGroupMetaData> RowGroupMetaData::Make(
    const uint8_t* metadata, const SchemaDescriptor* schema) {
  return std::unique_ptr<RowGroupMetaData>(new RowGroupMetaData(metadata, schema));
}

RowGroupMetaData::RowGroupMetaData(
    const uint8_t* metadata, const SchemaDescriptor* schema)
    : impl_{std::unique_ptr<RowGroupMetaDataImpl>(new RowGroupMetaDataImpl(
          reinterpret_cast<const format::RowGroup*>(metadata), schema))} {}
RowGroupMetaData::~RowGroupMetaData() {}

int RowGroupMetaData::num_columns() const {
  return impl_->num_columns();
}

int64_t RowGroupMetaData::num_rows() const {
  return impl_->num_rows();
}

int64_t RowGroupMetaData::total_byte_size() const {
  return impl_->total_byte_size();
}

const SchemaDescriptor* RowGroupMetaData::schema() const {
  return impl_->schema();
}

std::unique_ptr<ColumnChunkMetaData> RowGroupMetaData::ColumnChunk(int i) const {
  return impl_->ColumnChunk(i);
}

// file metadata
class FileMetaData::FileMetaDataImpl {
 public:
  FileMetaDataImpl() {}

  explicit FileMetaDataImpl(const uint8_t* metadata, uint32_t* metadata_len) {
    metadata_.reset(new format::FileMetaData);
    DeserializeThriftMsg(metadata, metadata_len, metadata_.get());
    InitSchema();
  }
  ~FileMetaDataImpl() {}

  inline int num_columns() const { return schema_.num_columns(); }
  inline int64_t num_rows() const { return metadata_->num_rows; }
  inline int num_row_groups() const { return metadata_->row_groups.size(); }
  inline int32_t version() const { return metadata_->version; }
  inline const std::string& created_by() const { return metadata_->created_by; }
  inline int num_schema_elements() const { return metadata_->schema.size(); }

  void WriteTo(OutputStream* dst) { SerializeThriftMsg(metadata_.get(), 1024, dst); }

  std::unique_ptr<RowGroupMetaData> RowGroup(int i) {
    if (!(i < num_row_groups())) {
      std::stringstream ss;
      ss << "The file only has " << num_row_groups()
         << " row groups, requested metadata for row group: " << i;
      throw ParquetException(ss.str());
    }
    return RowGroupMetaData::Make(
        reinterpret_cast<const uint8_t*>(&metadata_->row_groups[i]), &schema_);
  }

  const SchemaDescriptor* schema() const { return &schema_; }

 private:
  friend FileMetaDataBuilder;
  std::unique_ptr<format::FileMetaData> metadata_;
  void InitSchema() {
    schema::FlatSchemaConverter converter(
        &metadata_->schema[0], metadata_->schema.size());
    schema_.Init(converter.Convert());
  }
  SchemaDescriptor schema_;
};

std::unique_ptr<FileMetaData> FileMetaData::Make(
    const uint8_t* metadata, uint32_t* metadata_len) {
  return std::unique_ptr<FileMetaData>(new FileMetaData(metadata, metadata_len));
}

FileMetaData::FileMetaData(const uint8_t* metadata, uint32_t* metadata_len)
    : impl_{std::unique_ptr<FileMetaDataImpl>(
          new FileMetaDataImpl(metadata, metadata_len))} {}

FileMetaData::FileMetaData()
    : impl_{std::unique_ptr<FileMetaDataImpl>(new FileMetaDataImpl())} {}

FileMetaData::~FileMetaData() {}

std::unique_ptr<RowGroupMetaData> FileMetaData::RowGroup(int i) const {
  return impl_->RowGroup(i);
}

int FileMetaData::num_columns() const {
  return impl_->num_columns();
}

int64_t FileMetaData::num_rows() const {
  return impl_->num_rows();
}

int FileMetaData::num_row_groups() const {
  return impl_->num_row_groups();
}

int32_t FileMetaData::version() const {
  return impl_->version();
}

const std::string& FileMetaData::created_by() const {
  return impl_->created_by();
}

int FileMetaData::num_schema_elements() const {
  return impl_->num_schema_elements();
}

const SchemaDescriptor* FileMetaData::schema() const {
  return impl_->schema();
}

void FileMetaData::WriteTo(OutputStream* dst) {
  return impl_->WriteTo(dst);
}

// MetaData Builders
// row-group metadata
class ColumnChunkMetaDataBuilder::ColumnChunkMetaDataBuilderImpl {
 public:
  explicit ColumnChunkMetaDataBuilderImpl(const std::shared_ptr<WriterProperties>& props,
      const ColumnDescriptor* column, uint8_t* contents)
      : properties_(props), column_(column) {
    column_chunk_ = reinterpret_cast<format::ColumnChunk*>(contents);
    column_chunk_->meta_data.__set_type(ToThrift(column->physical_type()));
    column_chunk_->meta_data.__set_path_in_schema(column->path()->ToDotVector());
    column_chunk_->meta_data.__set_codec(
        ToThrift(properties_->compression(column->path())));
  }
  ~ColumnChunkMetaDataBuilderImpl() {}

  // column chunk
  void set_file_path(const std::string& val) { column_chunk_->__set_file_path(val); }

  // column metadata
  void SetStatistics(const ColumnStatistics& val) {
    format::Statistics stats;
    stats.null_count = val.null_count;
    stats.distinct_count = val.distinct_count;
    stats.max = *val.max;
    stats.min = *val.min;

    column_chunk_->meta_data.statistics = stats;
    column_chunk_->meta_data.__isset.statistics = true;
  }

  void Finish(int64_t num_values, int64_t dictionary_page_offset,
      int64_t index_page_offset, int64_t data_page_offset, int64_t compressed_size,
      int64_t uncompressed_size, bool dictionary_fallback = false) {
    if (dictionary_page_offset > 0) {
      column_chunk_->__set_file_offset(dictionary_page_offset + compressed_size);
    } else {
      column_chunk_->__set_file_offset(data_page_offset + compressed_size);
    }
    column_chunk_->__isset.meta_data = true;
    column_chunk_->meta_data.__set_num_values(num_values);
    column_chunk_->meta_data.__set_dictionary_page_offset(dictionary_page_offset);
    column_chunk_->meta_data.__set_index_page_offset(index_page_offset);
    column_chunk_->meta_data.__set_data_page_offset(data_page_offset);
    column_chunk_->meta_data.__set_total_uncompressed_size(uncompressed_size);
    column_chunk_->meta_data.__set_total_compressed_size(compressed_size);
    std::vector<format::Encoding::type> thrift_encodings;
    thrift_encodings.push_back(ToThrift(Encoding::RLE));
    if (properties_->dictionary_enabled(column_->path())) {
      thrift_encodings.push_back(ToThrift(properties_->dictionary_page_encoding()));
      // add the encoding only if it is unique
      if (properties_->version() == ParquetVersion::PARQUET_2_0) {
        thrift_encodings.push_back(ToThrift(properties_->dictionary_index_encoding()));
      }
    }
    if (!properties_->dictionary_enabled(column_->path()) || dictionary_fallback) {
      thrift_encodings.push_back(ToThrift(properties_->encoding(column_->path())));
    }
    column_chunk_->meta_data.__set_encodings(thrift_encodings);
  }

  const ColumnDescriptor* descr() const { return column_; }

 private:
  format::ColumnChunk* column_chunk_;
  const std::shared_ptr<WriterProperties> properties_;
  const ColumnDescriptor* column_;
};

std::unique_ptr<ColumnChunkMetaDataBuilder> ColumnChunkMetaDataBuilder::Make(
    const std::shared_ptr<WriterProperties>& props, const ColumnDescriptor* column,
    uint8_t* contents) {
  return std::unique_ptr<ColumnChunkMetaDataBuilder>(
      new ColumnChunkMetaDataBuilder(props, column, contents));
}

ColumnChunkMetaDataBuilder::ColumnChunkMetaDataBuilder(
    const std::shared_ptr<WriterProperties>& props, const ColumnDescriptor* column,
    uint8_t* contents)
    : impl_{std::unique_ptr<ColumnChunkMetaDataBuilderImpl>(
          new ColumnChunkMetaDataBuilderImpl(props, column, contents))} {}

ColumnChunkMetaDataBuilder::~ColumnChunkMetaDataBuilder() {}

void ColumnChunkMetaDataBuilder::set_file_path(const std::string& path) {
  impl_->set_file_path(path);
}

void ColumnChunkMetaDataBuilder::Finish(int64_t num_values,
    int64_t dictionary_page_offset, int64_t index_page_offset, int64_t data_page_offset,
    int64_t compressed_size, int64_t uncompressed_size, bool dictionary_fallback) {
  impl_->Finish(num_values, dictionary_page_offset, index_page_offset, data_page_offset,
      compressed_size, uncompressed_size, dictionary_fallback);
}

const ColumnDescriptor* ColumnChunkMetaDataBuilder::descr() const {
  return impl_->descr();
}

void ColumnChunkMetaDataBuilder::SetStatistics(const ColumnStatistics& result) {
  impl_->SetStatistics(result);
}

class RowGroupMetaDataBuilder::RowGroupMetaDataBuilderImpl {
 public:
  explicit RowGroupMetaDataBuilderImpl(int64_t num_rows,
      const std::shared_ptr<WriterProperties>& props, const SchemaDescriptor* schema,
      uint8_t* contents)
      : properties_(props), schema_(schema), current_column_(0) {
    row_group_ = reinterpret_cast<format::RowGroup*>(contents);
    InitializeColumns(schema->num_columns());
    row_group_->__set_num_rows(num_rows);
  }
  ~RowGroupMetaDataBuilderImpl() {}

  ColumnChunkMetaDataBuilder* NextColumnChunk() {
    if (!(current_column_ < num_columns())) {
      std::stringstream ss;
      ss << "The schema only has " << num_columns()
         << " columns, requested metadata for column: " << current_column_;
      throw ParquetException(ss.str());
    }
    auto column = schema_->Column(current_column_);
    auto column_builder = ColumnChunkMetaDataBuilder::Make(properties_, column,
        reinterpret_cast<uint8_t*>(&row_group_->columns[current_column_++]));
    auto column_builder_ptr = column_builder.get();
    column_builders_.push_back(std::move(column_builder));
    return column_builder_ptr;
  }

  void Finish(int64_t total_bytes_written) {
    if (!(current_column_ == schema_->num_columns())) {
      std::stringstream ss;
      ss << "Only " << current_column_ - 1 << " out of " << schema_->num_columns()
         << " columns are initialized";
      throw ParquetException(ss.str());
    }
    int64_t total_byte_size = 0;

    for (int i = 0; i < schema_->num_columns(); i++) {
      if (!(row_group_->columns[i].file_offset > 0)) {
        std::stringstream ss;
        ss << "Column " << i << " is not complete.";
        throw ParquetException(ss.str());
      }
      total_byte_size += row_group_->columns[i].meta_data.total_compressed_size;
    }
    DCHECK(total_bytes_written == total_byte_size)
        << "Total bytes in this RowGroup does not match with compressed sizes of columns";

    row_group_->__set_total_byte_size(total_byte_size);
  }

  int num_columns() { return row_group_->columns.size(); }

 private:
  void InitializeColumns(int ncols) { row_group_->columns.resize(ncols); }

  format::RowGroup* row_group_;
  const std::shared_ptr<WriterProperties> properties_;
  const SchemaDescriptor* schema_;
  std::vector<std::unique_ptr<ColumnChunkMetaDataBuilder>> column_builders_;
  int current_column_;
};

std::unique_ptr<RowGroupMetaDataBuilder> RowGroupMetaDataBuilder::Make(int64_t num_rows,
    const std::shared_ptr<WriterProperties>& props, const SchemaDescriptor* schema_,
    uint8_t* contents) {
  return std::unique_ptr<RowGroupMetaDataBuilder>(
      new RowGroupMetaDataBuilder(num_rows, props, schema_, contents));
}

RowGroupMetaDataBuilder::RowGroupMetaDataBuilder(int64_t num_rows,
    const std::shared_ptr<WriterProperties>& props, const SchemaDescriptor* schema_,
    uint8_t* contents)
    : impl_{std::unique_ptr<RowGroupMetaDataBuilderImpl>(
          new RowGroupMetaDataBuilderImpl(num_rows, props, schema_, contents))} {}

RowGroupMetaDataBuilder::~RowGroupMetaDataBuilder() {}

ColumnChunkMetaDataBuilder* RowGroupMetaDataBuilder::NextColumnChunk() {
  return impl_->NextColumnChunk();
}

int RowGroupMetaDataBuilder::num_columns() {
  return impl_->num_columns();
}

void RowGroupMetaDataBuilder::Finish(int64_t total_bytes_written) {
  impl_->Finish(total_bytes_written);
}

// file metadata
// TODO(PARQUET-595) Support key_value_metadata
class FileMetaDataBuilder::FileMetaDataBuilderImpl {
 public:
  explicit FileMetaDataBuilderImpl(
      const SchemaDescriptor* schema, const std::shared_ptr<WriterProperties>& props)
      : properties_(props), schema_(schema) {
    metadata_.reset(new format::FileMetaData());
  }
  ~FileMetaDataBuilderImpl() {}

  RowGroupMetaDataBuilder* AppendRowGroup(int64_t num_rows) {
    auto row_group = std::unique_ptr<format::RowGroup>(new format::RowGroup());
    auto row_group_builder = RowGroupMetaDataBuilder::Make(
        num_rows, properties_, schema_, reinterpret_cast<uint8_t*>(row_group.get()));
    RowGroupMetaDataBuilder* row_group_ptr = row_group_builder.get();
    row_group_builders_.push_back(std::move(row_group_builder));
    row_groups_.push_back(std::move(row_group));
    return row_group_ptr;
  }

  std::unique_ptr<FileMetaData> Finish() {
    int64_t total_rows = 0;
    std::vector<format::RowGroup> row_groups;
    for (auto row_group = row_groups_.begin(); row_group != row_groups_.end();
         row_group++) {
      auto rowgroup = *((*row_group).get());
      row_groups.push_back(rowgroup);
      total_rows += rowgroup.num_rows;
    }
    metadata_->__set_num_rows(total_rows);
    metadata_->__set_row_groups(row_groups);
    metadata_->__set_version(properties_->version());
    metadata_->__set_created_by(properties_->created_by());
    parquet::schema::SchemaFlattener flattener(
        static_cast<parquet::schema::GroupNode*>(schema_->schema_root().get()),
        &metadata_->schema);
    flattener.Flatten();
    auto file_meta_data = std::unique_ptr<FileMetaData>(new FileMetaData());
    file_meta_data->impl_->metadata_ = std::move(metadata_);
    file_meta_data->impl_->InitSchema();
    return file_meta_data;
  }

 protected:
  std::unique_ptr<format::FileMetaData> metadata_;

 private:
  const std::shared_ptr<WriterProperties> properties_;
  std::vector<std::unique_ptr<format::RowGroup>> row_groups_;
  std::vector<std::unique_ptr<RowGroupMetaDataBuilder>> row_group_builders_;
  const SchemaDescriptor* schema_;
};

std::unique_ptr<FileMetaDataBuilder> FileMetaDataBuilder::Make(
    const SchemaDescriptor* schema, const std::shared_ptr<WriterProperties>& props) {
  return std::unique_ptr<FileMetaDataBuilder>(new FileMetaDataBuilder(schema, props));
}

FileMetaDataBuilder::FileMetaDataBuilder(
    const SchemaDescriptor* schema, const std::shared_ptr<WriterProperties>& props)
    : impl_{std::unique_ptr<FileMetaDataBuilderImpl>(
          new FileMetaDataBuilderImpl(schema, props))} {}

FileMetaDataBuilder::~FileMetaDataBuilder() {}

RowGroupMetaDataBuilder* FileMetaDataBuilder::AppendRowGroup(int64_t num_rows) {
  return impl_->AppendRowGroup(num_rows);
}

std::unique_ptr<FileMetaData> FileMetaDataBuilder::Finish() {
  return impl_->Finish();
}

}  // namespace parquet
