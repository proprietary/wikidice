'use client';

import React from 'react';
import Downshift from 'downshift';
import { search } from '../api';
import styles from './page.module.css';

type AutocompleteProps = {
    query: string;
    placeholder: string;
    onSearch?: (query: string) => Promise<Array<string>>;
    handleQueryChange: (query: string) => void;
    onSubmit: (query: string) => Promise<void>;
};

type AutocompleteState = {
    searchResults: Array<string>;
};

export class Autocomplete extends React.Component<AutocompleteProps> {
    public state: AutocompleteState;

    constructor(props: AutocompleteProps) {
        super(props);
        this.state = {
            searchResults: [],
        };
    }

    componentDidMount() {
        this.setState({
            searchResults: [],
        });
    }

    handleChange = (e: React.ChangeEvent<HTMLInputElement>) => {
        const query = e.target.value;
        this.props.handleQueryChange(query);
    }

    handleSubmit = (e: React.FormEvent<HTMLFormElement>) => {
        e.preventDefault();
        this.props.onSubmit(this.props.query);
    }

    render() {
        return (
            <form onSubmit={this.handleSubmit}>
                <Downshift
                    onStateChange={({ inputValue }) => {
                        if (inputValue != null) {
                            (this.props.onSearch || search)(inputValue).then((searchResults) => {
                                this.setState({ searchResults });
                            });
                        }
                    }}
                    inputValue={this.props.query}
                    onChange={selection => {
                        this.props.handleQueryChange(selection);
                        this.props.onSubmit(selection);
                    }}
                    itemToString={(item) => (item == null ? '' : item)}
                >
                    {({
                        getInputProps,
                        getItemProps,
                        getLabelProps,
                        getMenuProps,
                        isOpen,
                        highlightedIndex,
                        selectedItem,
                    }) => (
                        <div>
                            <input
                                {...getInputProps({
                                    placeholder: "Physics",
                                    onChange: this.handleChange,
                                    value: this.props.query,
                                    className: styles.inputBox,
                                })}
                            />
                            <button type="submit" className={styles.searchButton}>🎲</button>
                            <ul {...getMenuProps()} style={{ listStyle: 'none' }}>
                                {isOpen
                                    ? this.state.searchResults
                                        .map((item, index) => (
                                            <li
                                                key={index}
                                                {...getItemProps({
                                                    index,
                                                    item,
                                                    style: {
                                                        fontWeight: selectedItem === item ? 'bold' : 'normal',
                                                        textDecoration: highlightedIndex === index ? 'underline' : 'none',
                                                    },
                                                })}
                                            >
                                                {item}
                                            </li>
                                        ))
                                    : null}
                            </ul>
                        </div>
                    )}
                </Downshift>
            </form>
        );
    }
}